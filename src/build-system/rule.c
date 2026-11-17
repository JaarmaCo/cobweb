// ================================================================================
// Copyright © 2026 William Jaarma
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ================================================================================

#include <limits.h>

#include "rule.h"
#include "compilation.h"

void preprocessor_arguments_inherit(preprocessor_arguments_t *dest, const preprocessor_arguments_t *src,
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

void linker_arguments_inherit(linker_arguments_t *dest, const linker_arguments_t *src,
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

void compiler_options_inherit(compiler_options_t *dest, const compiler_options_t *src,
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

void preprocessor_arguments_destroy(preprocessor_arguments_t *arguments) {
    if (!arguments) {
        return;
    }
    file_list_destroy(&arguments->includes);
    define_list_destroy(&arguments->definitions);
}

void linker_arguments_destroy(linker_arguments_t *arguments) {
    if (!arguments) {
        return;
    }
    sb_destroy(&arguments->rpath);
    file_list_destroy(&arguments->directories);
    file_list_destroy(&arguments->libraries);
}

void compiler_options_destroy(compiler_options_t *options) {
    if (!options) {
        return;
    }
    preprocessor_arguments_destroy(&options->preprocessor);
    linker_arguments_destroy(&options->linker);
}

void rule_destroy(rule_t *rule) {
    if (!rule) {
        return;
    }

    file_list_destroy(&rule->input);
    sb_destroy(&rule->output);
    sb_destroy(&rule->output_directory);
    dep_list_destroy(&rule->depends);
    compiler_options_destroy(&rule->options);
}


void build_context_defaults(build_context_t *context, env_t *env, allocator_t *allocator) {
    memset(context, 0, sizeof *context);
    
    context->env = env;
    context->status = stderr;
    context->command_builder.env = env;
    context->command_builder.allocator = allocator;
    context->output_directory = SV("out");
    context->compiler = compiler_gcc();
    context->allocator = allocator;
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
    string_view_t output_directory = rule->output_directory.count != 0
        ? sb_view(&rule->output_directory) : context->output_directory;
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

static bool file_exists_given_prefix(string_builder_t *sb, string_view_t prefix, string_view_t filename) {
    
    sb->count = 0;
    sb_append_sv(sb, prefix);
    if (!sv_ends_with_char(sb_view(sb), '/')) {
        sb_append_char(sb, '/');
    }
    sb_append_sv(sb, filename);

    return cmd_file_exists(sb->items); 
}

static bool resolve_input_via_dependencies(rule_t *rule, const rule_list_t *rules,
        build_context_t *context, string_builder_t *input) {
    
    bool found = false;
    string_builder_t output_rel_input = {.allocator = context->command_builder.allocator};

    for (size_t i = 0; i < rule->depends.count; ++i) {

        unsigned int dep_index = rule->depends.items[i];
        assert(dep_index > 0 && dep_index <= rules->count);

        const rule_t *dep = &rules->items[dep_index - 1];

        if (dep->output_directory.count == 0) {
            continue;
        }

        if (file_exists_given_prefix(&output_rel_input,
                    sb_view(&dep->output_directory),
                    sb_view(&rule->output))) {
            found = true;
            goto defer;
        }
    }

    if (file_exists_given_prefix(&output_rel_input,
                context->output_directory, sb_view(input))) {
        found = true;
        goto defer;
    }

defer:
    if (found) {
        input->count = 0;
        sb_append_sv(input, sb_view(&output_rel_input));
    }

    sb_destroy(&output_rel_input);
    return found;
}

bool run_rule(unsigned int number, const rule_list_t *rules, build_context_t *context) {

    bool dry_run = NULL != env_find(context->env, SV("DRY"));

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
        if (rule->type != RULE_OBJECT) {
            fprintf(context->status, "%.*s is up to date.\n",
                    (int)rule->output.count,
                    rule->output.items);
        }
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

    if (dry_run) {
        goto skip_validation;
    }

    for (size_t i = 0; i < rule->input.count; ++i) {

        string_builder_t *input = &rule->input.items[i];

        if (cmd_file_exists(input->items)) {
            continue;
        }
   
        if (resolve_input_via_dependencies(rule, rules, context, input)) {
            continue;
        }

        fprintf(context->status,
                "Missing input file %.*s required by %.*s.\n",
                (int)input->count, input->items,
                (int)rule->output.count, rule->output.items);
        goto err;
    }

    cmd_ensure_directory(rule->output_directory.items);

skip_validation:
    {
        command_t *command = run_rule_only(rule, context);

        if (!dry_run) {
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


