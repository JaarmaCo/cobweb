#include <limits.h>

#include "rule.h"
#include "allocator.h"
#include "command_builder.h"
#include "json.h"
#include "string_builder.h"

static bool rule_input_from_json(rule_t *rule, json_node_t node, FILE *status, allocator_t *allocator) {
    
    json_string_t contents;
    if (json_get(node, JSON_STRING, &contents)) {
        string_builder_t s = {.allocator = allocator};
        sb_append_sv(&s, sb_view(&contents));
        file_list_append(&rule->input, s);
        return true;
    }

    json_array_t array;
    if (!json_get(node, JSON_ARRAY, &array)) {
        fputs("\"input\" has to be a string, or an array of strings.\n", status);
        return false;
    }

    for (size_t i = 0; i < array.count; ++i) {
        json_node_t child = array.items[i];
        if (!json_get(child, JSON_STRING, &contents)) {
            fprintf(status, "Invalid value at element %zu in \"input\", expected a string.\n", i);
            return false;
        }
        string_builder_t sb = {.allocator = allocator};
        sb_append_sv(&sb, sb_view(&contents));
        file_list_append(&rule->input, sb);
    }
    return true;
}

static bool rule_output_from_json(rule_t *rule, json_node_t node, FILE *status) {
    
    json_string_t contents;
    if (json_get(node, JSON_STRING, &contents)) {
        sb_append_sv(&rule->output, sb_view(&contents)); 
        return true;
    }

    fputs("Expected \"output\" to be a string.\n", status);
    return false;
}

static bool rule_flags_from_json(rule_t *rule, json_node_t node, FILE *status, allocator_t *allocator) {
    
    json_array_t array;
    json_string_t contents;
    if (!json_get(node, JSON_ARRAY, &array)) {
        fputs("\"flags\" has to be an array of strings.\n", status);
        return false;
    }

    for (size_t i = 0; i < array.count; ++i) {
        json_node_t child = array.items[i];
        if (!json_get(child, JSON_STRING, &contents)) {
            fprintf(status, "Invalid value at element %zu in \"flags\", expected a string.\n", i);
            return false;
        }
        string_builder_t sb = {.allocator = allocator};
        sb_append_sv(&sb, sb_view(&contents));
        flags_append(&rule->flags, sb);
    }
    return true;
}

static bool rule_depends_from_json(rule_t *rule, json_node_t node, FILE *status) {
    
    json_array_t array;
    json_number_t contents;
    if (!json_get(node, JSON_ARRAY, &array)) {
        fputs("\"depends\" has to be an array of integers.\n", status);
        return false;
    }

    for (size_t i = 0; i < array.count; ++i) {
        json_node_t child = array.items[i];
        if (!json_get(child, JSON_NUMBER, &contents)) {
            fprintf(status, "Invalid value at element %zu in \"depends\", expected a number.\n", i);
            return false;
        }

        if (contents == 0) {
            fprintf(status, "Dependency indices are 1-indexed, "
                    "value at position %zu is 0 (in array \"depends\").\n", i);
            return false;
        }

        if (contents < 0) {
            fprintf(status, "Negative dependency index at position %zu in \"depends\".\n", i);
            return false;
        }

        if (contents > UINT_MAX) {
            fprintf(status, "Dependency index exceeds maximum limit at position %zu in \"depends\".\n", i);
            return false;
        }

        if ((json_number_t)(unsigned int)contents != contents) {
            fprintf(status, "Expected an integer at position %zu in \"depends\".\n", i);
            return false;
        }

        dep_list_append(&rule->depends, (unsigned int)contents);
    }
    return true;
}

static bool rule_linker_from_json(rule_t *rule, json_node_t node, FILE *status, allocator_t *allocator) {
    
    if (node.type != JSON_OBJECT) {
        fputs("Expected property \"linker\" to be an object.\n", status);
        return false;
    }

    json_string_t string;
    json_array_t array;
    json_node_t *child;

    child = json_find(node, SV("rpath"));
    if (child) {
        if (!json_get(*child, JSON_STRING, &string)) {
            fputs("Expected property \"rpath\" of object \"linker\" to be a string.",
                    status);
            return false;
        }
        string_builder_t sb = {.allocator = allocator};
        sb_append_sv(&sb, sb_view(&string));
        rule->linker.rpath = sb;
    }

    child = json_find(node, SV("directories"));
    if (child) {
        if (!json_get(*child, JSON_ARRAY, &array)) {
            fputs("Expected property \"directories\" of object \"linker\" to be an array of strings.\n", status);
            return false;
        }

        for (size_t i = 0; i < array.count; ++i) {
            string_builder_t sb = {.allocator = allocator};
            if (!json_get(array.items[i], JSON_STRING, &string)) {
                fprintf(status, "Expected element at position %zu in array "
                        "\"directories\" of object \"linker\" to be a string.\n", i);
                return false;
            }
            sb_append_sv(&sb, sb_view(&string));
            file_list_append(&rule->linker.directories, sb);
        }
    }

    child = json_find(node, SV("libraries"));
    if (child) {
        if (!json_get(*child, JSON_ARRAY, &array)) {
            fputs("Expected property \"libraries\" of object \"linker\" to be an array of strings.\n", status);
            return false;
        }

        for (size_t i = 0; i < array.count; ++i) {
            string_builder_t sb = {.allocator = allocator};
            if (!json_get(array.items[i], JSON_STRING, &string)) {
                fprintf(status, "Expected element at position %zu in array "
                        "\"libraries\" of object \"linker\" to be a string.\n", i);
                return false;
            }
            sb_append_sv(&sb, sb_view(&string));
            file_list_append(&rule->linker.libraries, sb);
        }
    }

    return true;
}

static bool rule_preprocessor_from_json(rule_t *rule, json_node_t node, FILE *status, allocator_t *allocator) {
    
    if (node.type != JSON_OBJECT) {
        fputs("Expected property \"preprocessor\" to be an object.\n", status);
        return false;
    }

    json_string_t string;
    json_array_t array;
    json_node_t *child;

    child = json_find(node, SV("includes"));
    if (child) {
        if (!json_get(*child, JSON_ARRAY, &array)) {
            fputs("Expected property \"includes\" of object \"preprocessor\" to be an array of strings.\n", status);
            return false;
        }

        for (size_t i = 0; i < array.count; ++i) {
            string_builder_t sb = {.allocator = allocator};
            if (!json_get(array.items[i], JSON_STRING, &string)) {
                fprintf(status, "Expected element at position %zu in array "
                        "\"includes\" of object \"preprocessor\" to be a string.\n", i);
                return false;
            }
            sb_append_sv(&sb, sb_view(&string));
            file_list_append(&rule->preprocessor.includes, sb);
        }
    }

    child = json_find(node, SV("definitions"));
    if (child) {
        if (child->type != JSON_OBJECT) {
            fputs("Expected property \"definitions\" of object \"preprocessor\" "
                    "to be an object of symbol-expansion pairs.\n", status);
            return false;
        }

        size_t state = 0;
        string_view_t key;
        json_node_t value;
        while (json_iterate_properties(*child, &state, &key, &value)) {

            preprocessor_definition_t def = {
                .symbol = { .allocator = allocator },
                .expansion = { .allocator = allocator },
            };

            sb_append_sv(&def.symbol, key);

            if (json_get(value, JSON_STRING, &string)) {
                sb_append_sv(&def.expansion, sb_view(&string));
            } else if (value.type != JSON_NULL) {
                fprintf(status, "Expected property \"%.*s\" of object \"definitions\" "
                        "in object \"preprocessor\" to be eithr a string or null.\n", 
                        (int)key.count, key.items);
                return false;
            }

            define_list_append(&rule->preprocessor.definitions, def);
        }
    }
    return true;
}

bool rule_from_json(rule_t *rule, json_node_t node, FILE *status, allocator_t *allocator) {
    
    if (node.type != JSON_OBJECT) {
        fputs("Expected the root element to be an object.\n", status);
        return false;
    }

    json_node_t *child;
    memset(rule, 0, sizeof *rule);
    
    allocator = auto_free_allocator(allocator);

    rule->allocator = allocator;
    rule->input.allocator = allocator;
    rule->output.allocator = allocator;
    rule->flags.allocator = allocator;
    rule->depends.allocator = allocator;
    rule->linker.rpath.allocator = allocator;
    rule->linker.directories.allocator = allocator;
    rule->linker.libraries.allocator = allocator;
    rule->preprocessor.definitions.allocator = allocator;
    rule->preprocessor.includes.allocator = allocator;

    child = json_find(node, SV("input"));
    if (!child) {
        fputs("Missing required \"input\" property.\n", status);
        return false;
    }
    if (!rule_input_from_json(rule, *child, status, allocator)) {
        return false;
    }

    child = json_find(node, SV("output"));
    if (child) {
        if (!rule_output_from_json(rule, *child, status)) {
            return false;
        }
    } else {
        fputs("Missing required property \"output\".\n\n", status);
        return false;
    }

    child = json_find(node, SV("type"));
    if (child) {
        json_string_t string;
        if (!json_get(*child, JSON_STRING, &string)) {
            fputs("Expected property \"type\" to be a string.\n\n", status);
            return false;
        }
        
        if (sv_equals(sb_view(&string), SV("object"))) {
            rule->type = RULE_OBJECT;
        } else if (sv_equals(sb_view(&string), SV("executable"))) {
            rule->type = RULE_EXECUTABLE;
        } else if (sv_equals(sb_view(&string), SV("static-library"))) {
            rule->type = RULE_STATIC_LIBRARY;
        } else if (sv_equals(sb_view(&string), SV("dynamic-library"))) {
            rule->type = RULE_DYNAMIC_LIBRARY;
        } else {
            fputs("Expected property \"type\" to be one of [ \"object\", "
                    "\"executable\", \"static-library\", \"dynamic-library\" ].\n", status);
            return false;
        }

    } else {
        fputs("Missing required property \"type\".\n\n", status);
        return false;
    }

    child = json_find(node, SV("flags"));
    if (child) {
        if (!rule_flags_from_json(rule, *child, status, allocator)) {
            return false;
        }
    }

    child = json_find(node, SV("depends"));
    if (child) {
        if (!rule_depends_from_json(rule, *child, status)) {
            return false;
        }
    }

    child = json_find(node, SV("linker"));
    if (child) {
        if (!rule_linker_from_json(rule, *child, status, allocator)) {
            return false;
        }
    }

    child = json_find(node, SV("preprocessor"));
    if (child) {
        if (!rule_preprocessor_from_json(rule, *child, status, allocator)) {
            return false;
        }
    }
    return true;
}

bool rule_list_from_json(rule_list_t *list, json_node_t json, FILE *status, allocator_t *allocator) { 
    json_array_t array;
    if (!json_get(json, JSON_ARRAY, &array)) {
        fputs("Expected a list as the root object.\n", status);
        return false;
    }
    memset(list, 0, sizeof *list);
    list->allocator = allocator;
    for (size_t i = 0; i < array.count; ++i) {
        rule_t rule = {0};
        if (!rule_from_json(&rule, array.items[i], status, allocator)) {
            rule_destroy(&rule);
            fprintf(status, "When parsing rule at position %zu.\n", i);
            return false;
        }
        rule_list_append(list, rule);
    }
    return true;
}

void rule_destroy(rule_t *rule) {
    
    if (!rule) {
        return;
    }

    auto_free_destroy(rule->allocator);
    memset(rule, 0, sizeof *rule);
}

command_t *run_rule_only(const rule_t *rule, build_context_t *context) {
    return context->compiler.callback(rule, context, context->compiler.arg);
}

struct rule_async_runner {
    unsigned int number;
    rule_list_t rules;
    build_context_t context;
};

static void run_rule_async_runner(void *arg) {
    struct rule_async_runner *runner = (struct rule_async_runner *)arg;
    if (run_rule(runner->number, &runner->rules, &runner->context)) {
        exit(0);
    }
    exit(1);
}

command_t *run_rule_async(unsigned int number, const rule_list_t *rules, build_context_t *context) {
    struct rule_async_runner runner = {
        .number = number,
        .rules = *rules,
        .context = *context,
    };
    return cmd_async(run_rule_async_runner, &runner);
}

static bool rule_needs_rebuild(const rule_t *rule, build_context_t *context) {
    
    bool result = false;
    string_view_t output_directory = context->output_directory;
    string_builder_t output = {.allocator = context->command_builder.allocator};

    sb_append_sv(&output, output_directory);
    if (!sv_ends_with_char(sb_view(&output), '/')) {
        sb_append_char(&output, '/');
    }

    sb_append_sv(&output, sb_view(&rule->output));

    if (!cmd_file_exists(output.items)) {
        result = true;
        goto defer;
    }

    for (size_t i = 0; i < rule->input.count; ++i) {
        double diff_time = cmd_file_age_difference(output.items, rule->input.items[i].items);
        if (diff_time < 0) {
            result = true;
            goto defer;
        }
    }

defer:
    sb_destroy(&output);
    return result;
}

static bool run_dependencies_concurrently(const rule_t *rule, const rule_list_t *rules, build_context_t *context) {
    unsigned int dep = 0;
    int concurrency = context->concurreny == 0 ? 1 : context->concurreny;
    concurrency = concurrency < RULE_MAX_CONCURRENCY ? concurrency : RULE_MAX_CONCURRENCY;
    while (dep < rule->depends.count) {

        command_t *running[RULE_MAX_CONCURRENCY] = {0};
        int exit_codes[RULE_MAX_CONCURRENCY] = {0};

        for (int i = 0; i < concurrency; ++i) {
            command_t *command = run_rule_async(rule->depends.items[i], rules, context);
            if (!command) {
                cmd_wait_every((size_t)i, running, exit_codes);
                return false;
            }
            running[i] = command;
            ++dep;
        }

        cmd_wait_every((size_t)concurrency, running, exit_codes);
    
        for (int i = 0; i < concurrency; ++i) {
            if (exit_codes[i] != 0) {
                fprintf(context->status,
                        "Subprocess of rule dependency exited with a nonzero exit code.\n");
                return false;
            }
        }
    }
    return true;
}

bool run_rule(unsigned int number, const rule_list_t *rules, build_context_t *context) {
    
    if (number == 0 || number > rules->count) {
        fprintf(context->status, "Rule number %u is outside the valid range [1, %u]\n",
                number, (unsigned int)rules->count);
        return false;
    }
    
    if (context->dependency_depth >= RULE_MAX_DEPENDENCY_DEPTH) {
        fprintf(context->status, "Maximum dependency depth reached. Possibly a cycle.\n");
        return false;
    }

    rule_t *rule = &rules->items[number - 1];
    if (!rule_needs_rebuild(rule, context)) {
        fprintf(context->status, "%.*s is up to date.\n",
                (int)rule->output.count,
                rule->output.items);
        return true;
    }

    ++context->dependency_depth;

    if (context->concurreny > 1) {
        if (!run_dependencies_concurrently(rule, rules, context)) {
            goto err;
        }
    } else {
        for (size_t i = 0; i < rule->depends.count; ++i) {
            if (!run_rule(rule->depends.items[i], rules, context)) {
                goto err;
            }
        }
    }

    for (size_t i = 0; i < rule->input.count; ++i) {

        string_builder_t input = rule->input.items[i];

        if (cmd_file_exists(input.items)) {
            continue;
        }
        
        string_builder_t output_rel_input = {.allocator = context->command_builder.allocator};
        sb_append_sv(&output_rel_input, context->output_directory);
        if (!sv_ends_with_char(sb_view(&output_rel_input), '/')) {
            sb_append_char(&output_rel_input, '/');
        }
        sb_append_sv(&output_rel_input, sb_view(&input));

        if (cmd_file_exists(output_rel_input.items)) {
            sb_destroy(&input);
            rule->input.items[i] = output_rel_input;
            continue;
        }

        sb_destroy(&output_rel_input);

        fprintf(context->status,
                "Missing input file %.*s required by %.*s.\n",
                (int)rule->input.items[i].count,
                rule->input.items[i].items,
                (int)rule->output.count,
                rule->output.items);
        goto err;
    }

    cmd_ensure_directory(sv_to_temp_cstr(context->output_directory));

    {
        command_t *command = run_rule_only(rule, context);
        if (!command) {
            goto err;
        }

        int exit_code = cmd_wait(command);
        if (exit_code != 0) {
            fprintf(context->status,
                    "Failed to produce %.*s, subprocess exited with a nonzero exit code.\n",
                    (int)rule->output.count,
                    rule->output.items);
            goto err;
        }
    }

    --context->dependency_depth;
    return true;
err:
    --context->dependency_depth;
    return false;
}

#define TYPE_0 file_list_t
#define TYPE_1 string_builder_t
#define PREFIX file_list_
#define SUFFIX
#define C_SOURCE
#define FUNCTION_0 sb_destroy
#include "../dynamic_array.h"

#define TYPE_0 dependency_list_t
#define TYPE_1 unsigned int
#define PREFIX dep_list_
#define SUFFIX
#define C_SOURCE
#include "../dynamic_array.h"

#define TYPE_0 preprocessor_definition_list_t
#define TYPE_1 preprocessor_definition_t
#define PREFIX define_list_
#define SUFFIX
#define C_SOURCE
#define FUNCTION_0(def) do { sb_destroy(&(def)->symbol); sb_destroy(&(def)->expansion); } while (0)
#include "../dynamic_array.h"

#define TYPE_0 flags_list_t
#define TYPE_1 string_builder_t
#define PREFIX flags_
#define SUFFIX
#define C_SOURCE
#define FUNCTION_0 sb_destroy
#include "../dynamic_array.h"

#define TYPE_0 rule_list_t
#define TYPE_1 rule_t
#define PREFIX rule_list_
#define SUFFIX
#define C_SOURCE
#define FUNCTION_0 rule_destroy
#include "../dynamic_array.h"


