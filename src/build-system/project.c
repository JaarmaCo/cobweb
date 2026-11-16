#include "project.h"
#include "build-system/rule.h"
#include "command_builder.h"
#include "string_builder.h"

static void index_build_target(const build_target_t *target, const build_project_t *project,
        project_generator_context_t *context) {

    string_builder_t sb = {.allocator = context->allocator};
    sb_append_sv(&sb, sb_view(&target->name));
    target_dependency_map_insert(&context->indexed_targets, sb, (unsigned int)(project->targets.items - target) + 1);
}

static build_target_t *query_build_target(const string_builder_t *name,
        const build_project_t *project, project_generator_context_t *context) {

    target_dependency_t *existing = target_dependency_map_find(&context->indexed_targets, *name);
    if (existing) {
        return &project->targets.items[existing->value - 1];
    }

    build_target_t *dep = NULL;
    for (size_t i = 0; i < project->targets.count; ++i) {
        if (sv_equals(sb_view(name), sb_view(&project->targets.items[i].name))) {
            dep = &project->targets.items[i];
            index_build_target(dep, project, context);
            break;
        }
    }
    return dep;
}

// Find artifact using a lookup string of the format: target#artifact
static unsigned int query_artifact_rule_index(const string_builder_t *resource, project_generator_context_t *context) {
    target_dependency_t *existing = target_dependency_map_find(&context->artifact_rules, *resource);
    if (existing) {
        return existing->value;
    }
    return 0;
}

// Cascade rule generation to transitive dependencies
static unsigned int add_transitive_dependency(const string_builder_t *name,
        const build_target_t *target, const build_project_t *project,
        rule_list_t *rules, project_generator_context_t *context) {
    
    build_target_t *dep = query_build_target(name, project, context);
    if (!dep) {
        fprintf(context->status,
                "Missing target %.*s required by %.*s.\n",
                (int)name->count, name->items,
                (int)target->name.count, target->name.items);
        return 0;
    }

    unsigned int index = 0;
    string_builder_t artifact_rule_name = {.allocator = context->allocator};
    sb_format(&artifact_rule_name, "%.*s#%.*s",
            (int)dep->name.count, dep->name.items,
            (int)dep->output.count, dep->output.items);

    index = query_artifact_rule_index(&artifact_rule_name, context);
    if (index != 0) {
        goto defer;
    }

    ++context->dependency_depth;
    index = build_target_emit_rules(dep, project, rules, context);
    --context->dependency_depth;

defer:
    sb_destroy(&artifact_rule_name);
    return index;
}

static void preprocessor_arguments_inherit(preprocessor_arguments_t *dest, const preprocessor_arguments_t *src,
        allocator_t *allocator) {
    
    for (size_t i = 0; i < src->includes.count; ++i) {
        string_builder_t sb = {.allocator = allocator};
        sb_append_sv(&sb, sb_view(&src->includes.items[i]));
        file_list_append(&dest->includes, sb);
    }
    
    for (size_t i = 0; i < src->definitions.count; ++i) {
        preprocessor_definition_t def = {
            .symbol = {.allocator = allocator},
            .expansion = {.allocator = allocator},
        };
        sb_append_sv(&def.symbol, sb_view(&src->definitions.items[i].symbol));
        sb_append_sv(&def.expansion, sb_view(&src->definitions.items[i].expansion));
        define_list_append(&dest->definitions, def);
    }
}

static void linker_arguments_inherit(linker_arguments_t *dest, const linker_arguments_t *src,
        allocator_t *allocator) {
    
    dest->rpath.allocator = allocator;
    sb_append_sv(&dest->rpath, sb_view(&src->rpath));

    for (size_t i = 0; i < src->libraries.count; ++i) {
        string_builder_t sb = {.allocator = allocator};
        sb_append_sv(&sb, sb_view(&src->libraries.items[i]));
        file_list_append(&dest->libraries, sb);
    }
    
    for (size_t i = 0; i < src->directories.count; ++i) {
        string_builder_t sb = {.allocator = allocator};
        sb_append_sv(&sb, sb_view(&src->directories.items[i]));
        file_list_append(&dest->directories, sb);
    }       
}

static void compiler_options_inherit(compiler_options_t *dest, const compiler_options_t *src,
        allocator_t *allocator) {

    if (src->optimization > dest->optimization) {
        dest->optimization = src->optimization;
    }

    if (src->warnings > dest->warnings) {
        dest->warnings = src->warnings;
    }

    dest->switches |= src->switches;
    preprocessor_arguments_inherit(&dest->preprocessor, &src->preprocessor, allocator);
    linker_arguments_inherit(&dest->linker, &src->linker, allocator);
}

static bool locate_target_source_file(string_builder_t *absolute,
        const build_target_t *target, const string_builder_t *source_name) {
    
    absolute->count = 0;
    if (cmd_file_exists(source_name->items)) {
        sb_append_sv(absolute, sb_view(source_name));
        return true;
    }

    for (size_t i = 0; i < target->source_directories.count; ++i) {
        absolute->count = 0;
        string_view_t dir = sb_view(&target->source_directories.items[i]);
        sb_append_sv(absolute, dir);
        if (!sv_ends_with_char(dir, '/')) {
            sb_append_char(absolute, '/');
        }
        sb_append_sv(absolute, sb_view(source_name));
        if (cmd_file_exists(absolute->items)) {
            return true;
        }
    }
    absolute->count = 0;
    return false;
}

static void infer_target_artifact(string_builder_t *name, const build_target_t *target) {
    switch (target->type)
    {
    case RULE_OBJECT:
        sb_format(name, "%.*s.o",
                (int)target->name.count, target->name.items);
        break;
    case RULE_EXECUTABLE:
        sb_append_sv(name, sb_view(&target->name));
        break;
    case RULE_DYNAMIC_LIBRARY:
        sb_format(name, "lib%.*s.so",
                (int)target->name.count, target->name.items);
        break;
    case RULE_STATIC_LIBRARY:
        sb_format(name, "lib%.*s.a",
                (int)target->name.count, target->name.items);
        break;
    }
}

// Function that emits the actual rules
static unsigned int target_define_rule(const build_target_t *target, const rule_t *rule,
        rule_list_t *rules, project_generator_context_t *context) {
    
    rule_list_append(rules, *rule);

    unsigned int index = (unsigned int)rules->count;
    string_builder_t resource_name = {.allocator = context->allocator};
    sb_format(&resource_name, "%.*s#%.*s",
            (int)target->name.count, target->name.items,
            (int)rule->output.count, rule->output.items);
    
    target_dependency_map_insert(&context->artifact_rules,
            resource_name, index);
    
    return index;
}

struct process_dependency_args {
    project_generator_context_t *context;
    rule_list_t *rules;
    dependency_list_t *dep_list;
    file_list_t *dep_files;
    compiler_options_t *dep_options;
    const build_project_t *project;
    const build_target_t *target;
};

static bool process_dependency(const string_builder_t *dep_name, 
        struct process_dependency_args *args) {

    const build_target_t *dep_target = query_build_target(dep_name, args->project, args->context);
    unsigned int index = add_transitive_dependency(dep_name, args->target, args->project, args->rules, args->context);
    if (index == 0) {
        return false;
    }
    dep_list_append(args->dep_list, index);

    compiler_options_inherit(args->dep_options, &args->target->options.external, args->context->allocator);
    compiler_options_inherit(args->dep_options, &args->target->options.shared, args->context->allocator);

    switch (dep_target->type)
    {
    case RULE_OBJECT:
    case RULE_STATIC_LIBRARY: {
        string_builder_t sb = {.allocator = args->context->allocator};
        sb_append_sv(&sb, sb_view(&dep_target->output));
        file_list_append(args->dep_files, sb);
    }
        break;
    case RULE_DYNAMIC_LIBRARY: {
        string_builder_t sb = {.allocator = args->context->allocator};
        sb_append_sv(&sb, sb_view(&dep_target->output));
        file_list_append(&args->dep_options->linker.libraries, sb);    
    }
        break;
    default:
        break;
    }
    return true;
}

unsigned int build_target_emit_rules(const build_target_t *target, const build_project_t *project,
        rule_list_t *rules, project_generator_context_t *context) {

    if (context->dependency_depth >= RULE_MAX_DEPENDENCY_DEPTH) {
        fputs("Maxium dependency depth reached. Possibly a cycle.\n", context->status);
        return 0;
    }

    {
        target_dependency_t *dep = target_dependency_map_find(&context->completed_targets, target->name);
        if (dep) {
            return dep->value;
        }
    }

    // List of all-round dependencies
    dependency_list_t dep_list = {.allocator = context->allocator};
    
    // Intermediates generated by this target
    dependency_list_t local_dep_list = {.allocator = context->allocator};
    
    // Files that need to be part of the target input list
    file_list_t dep_files = {.allocator = context->allocator};

    // Compiler options to inherit for all target rules
    compiler_options_t dep_options = compiler_options(context->allocator); 

    compiler_options_inherit(&dep_options, &project->options, context->allocator);
    compiler_options_inherit(&dep_options, &target->options.internal, context->allocator);
    compiler_options_inherit(&dep_options, &target->options.shared, context->allocator);

    index_build_target(target, project, context);

    for (size_t i = 0; i < target->depends.count; ++i) {
        struct process_dependency_args args = {
            .dep_list = &dep_list,
            .dep_files = &dep_files,
            .dep_options = &dep_options,
            .context = context,
            .project = project,
            .target = target,
            .rules = rules,
        };
        if (!process_dependency(&target->depends.items[i], &args)) {
            goto err;
        }
    }

    // Add rules for intermediate files
    for (size_t i = 0; i < target->sources.count; ++i) {
        string_builder_t source_name = {.allocator = context->allocator};
        if (!locate_target_source_file(&source_name, target, &target->sources.items[i])) {
            fprintf(stderr,
                    "Could not find source file %.*s required by target %.*s\n",
                    (int)target->sources.items[i].count, target->sources.items[i].items,
                    (int)target->name.count, target->name.items);
            sb_destroy(&source_name);
            goto err;
        }
        rule_t rule = {0};
        rule.type = RULE_OBJECT;
        rule.input.allocator = context->allocator;
        rule.options = compiler_options(context->allocator);
        rule.depends.allocator = context->allocator;
        rule.output.allocator = context->allocator;
        
        {
            size_t dot = sv_rfind_char(sb_view(&source_name), '.', 0);
            string_view_t source_name_base = sv_take(sb_view(&source_name),
                    dot == SIZE_MAX ? source_name.count : dot);
            
            sb_format(&rule.output, "%.*s.o",
                    (int)source_name_base.count, source_name_base.items);
        }

        file_list_append(&rule.input, source_name);

        dep_list_append_range(&rule.depends, dep_list.count, dep_list.items);
        
        compiler_options_inherit(&rule.options, &dep_options, context->allocator);

        dep_list_append(&local_dep_list, (unsigned int)rules->count + 1);    
        target_define_rule(target, &rule, rules, context);
    }

    // Build the final target rule
    unsigned int index;
    {
        rule_t rule = {0};
        rule.type = target->type;
        rule.input.allocator = context->allocator;
        rule.options = compiler_options(context->allocator);
        rule.depends.allocator = context->allocator;
        rule.output.allocator = context->allocator;
       
        if (target->output.count != 0) {
            sb_append_sv(&rule.output, sb_view(&target->output));
        } else {
            infer_target_artifact(&rule.output, target);
        }

        compiler_options_inherit(&rule.options, &dep_options, context->allocator);

        dep_list_append_range(&rule.depends, dep_list.count, dep_list.items);
        dep_list_append_range(&rule.depends, local_dep_list.count, local_dep_list.items);

        for (size_t i = 0; i < dep_files.count; ++i) {
            string_builder_t file = {.allocator = context->allocator};
            sb_append_sv(&file, sb_view(&dep_files.items[i]));
            file_list_append(&rule.input, file);
        }

        for (size_t i = 0; i < local_dep_list.count; ++i) {
            unsigned int dep_index = local_dep_list.items[i];
            rule_t *dep = &rules->items[dep_index - 1];
            string_builder_t file = {.allocator = context->allocator};
            sb_append_sv(&file, sb_view(&dep->output));
            file_list_append(&rule.input, file);
        }

        index = target_define_rule(target, &rule, rules, context);
    }

    // Ensure the target is marked as already done
    {
        string_builder_t tname = {.allocator = context->allocator};
        sb_append_sv(&tname, sb_view(&target->name));
        target_dependency_map_insert(&context->completed_targets, tname, index);
    }

    dep_list_destroy(&local_dep_list);
    dep_list_destroy(&dep_list);
    file_list_destroy(&dep_files);
    compiler_options_destroy(&dep_options);
    return index;
err:
    dep_list_destroy(&local_dep_list);
    dep_list_destroy(&dep_list);
    file_list_destroy(&dep_files);
    compiler_options_destroy(&dep_options);
    return 0;
}

bool build_project_emit_rules(const build_project_t *project,
        rule_list_t *rules, project_generator_context_t *context) {

    for (size_t i = 0; i < project->targets.count; ++i) {
        if (!build_target_emit_rules(&project->targets.items[i],
                    project, rules, context)) {
            return false;
        }
    }
    return true;
}

void target_compiler_options_destroy(target_compiler_options_t *value) {
    if (!value) {
        return;
    }
    compiler_options_destroy(&value->shared);
    compiler_options_destroy(&value->external);
    compiler_options_destroy(&value->internal);
}

void build_target_destroy(build_target_t *value) {
    if (!value) {
        return;
    }
    sb_destroy(&value->name);
    sb_destroy(&value->output);
    file_list_destroy(&value->source_directories);
    file_list_destroy(&value->sources);
    file_list_destroy(&value->test_directories);
    file_list_destroy(&value->tests);
    file_list_destroy(&value->depends);
    target_compiler_options_destroy(&value->options);
}

void build_project_destroy(build_project_t *value) {
    if (!value) {
        return;
    }
    file_list_destroy(&value->source_directories);
    file_list_destroy(&value->test_directories);
    compiler_options_destroy(&value->options);
    build_target_list_destroy(&value->targets);
}

#define TYPE_0 build_target_list_t
#define TYPE_1 build_target_t
#define FUNCTION_0 build_target_destroy
#define PREFIX build_target_list_
#define SUFFIX 
#define C_SOURCE
#include "../dynamic_array.h"

#define TYPE_0 target_dependency_map_t
#define TYPE_1 target_dependency_t
#define TYPE_2 string_builder_t
#define TYPE_3 unsigned int
#define FUNCTION_0(e) sb_destroy(&(e)->key)
#define FUNCTION_1(k) sv_hash(sb_view(&k))
#define FUNCTION_2(x, y) sv_equals(sb_view(&x), sb_view(&y))
#define PREFIX target_dependency_map_
#define SUFFIX
#define C_SOURCE
#include "../hashmap.h"


