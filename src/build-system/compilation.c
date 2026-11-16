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

#include "compilation.h"
#include <stddef.h>

struct gcc_compiler {
    bool found_gcc;
    command_builder_t cc;
    command_builder_t ar;

    _Alignas(max_align_t) char buffer[4096];
} gcc_compiler;

static void gcc_apply_options(const rule_t *rule, build_context_t *build) {
    
    if (rule->options.switches & COMPILER_EMIT_DEBUG_SYMBOLS) {
        cmd_append(&build->command_builder, "-ggdb");
    }

    if (rule->options.switches & COMPILER_PIC) {
        cmd_append(&build->command_builder, "-fPIC");
    }

    switch (rule->options.optimization)
    {
    case COMPILER_OPTIMIZATION_OFF:
        cmd_append(&build->command_builder, "-O0");
        break;
    case COMPILER_OPTIMIZATION_LOW:
        cmd_append(&build->command_builder, "-Og");
        break;
    case COMPILER_OPTIMIZATION_MODERATE:
        cmd_append(&build->command_builder, "-O");
        break;
    case COMPILER_OPTIMIZATION_HIGH:
        cmd_append(&build->command_builder, "-O2");
        break;
    case COMPILER_OPTIMIZATION_ADVANCED:
        cmd_append(&build->command_builder, "-O3");
        break;
    case COMPILER_OPTIMIZATION_AGRESSIVE:
        cmd_append(&build->command_builder, "-Ofast");
        break;
    }
    
    switch (rule->options.warnings)
    {
    case COMPILER_WARNINGS_OFF:
        break;
    case COMPILER_WARNINGS_BASIC:
        cmd_append(&build->command_builder, "-Wall");
        break;
    case COMPILER_WARNINGS_MODERATE:
        cmd_append_all(&build->command_builder, "-Wall", "-Wextra", NULL);
        break;
    case COMPILER_WARNINGS_STRICT:
        cmd_append_all(&build->command_builder, "-Wall", "-Wextra", "-Wconversion", NULL);
        break;
    case COMPILER_WARNINGS_PEDANTIC:
        cmd_append_all(&build->command_builder, "-Wall", "-Wextra", "-Wconversion", "-Wpedantic", NULL);
        break;
    }

    if (rule->options.switches & COMPILER_TREAT_WARNINGS_AS_ERRORS) {
        cmd_append(&build->command_builder, "-Werror");
    }

    for (size_t i = 0; i < rule->options.linker.directories.count; ++i) {
        cmd_format(&build->command_builder, "-L%.*s",
                (int)rule->options.linker.directories.items[i].count,
                rule->options.linker.directories.items[i].items);
    }

    for (size_t i = 0; i < rule->options.linker.libraries.count; ++i) {
        cmd_format(&build->command_builder, "-l%.*s",
                (int)rule->options.linker.libraries.items[i].count,
                rule->options.linker.libraries.items[i].items);
    }

    if (rule->options.linker.rpath.count != 0) {
        cmd_format(&build->command_builder, "-rpath=%.*s",
                (int)rule->options.linker.rpath.count,
                rule->options.linker.rpath.items);
    }

    for (size_t i = 0; i < rule->options.preprocessor.includes.count; ++i) {
        cmd_format(&build->command_builder, "-I%.*s",
                (int)rule->options.preprocessor.includes.items[i].count,
                rule->options.preprocessor.includes.items[i].items);
    }

    for (size_t i = 0; i < rule->options.preprocessor.definitions.count; ++i) {
        preprocessor_definition_t *def = &rule->options.preprocessor.definitions.items[i];
        if (def->expansion.count != 0) {
            cmd_format(&build->command_builder, "-D%.*s=%.*s",
                    (int)def->symbol.count,
                    def->symbol.items,
                    (int)def->expansion.count,
                    def->expansion.items);
        } else {
            cmd_format(&build->command_builder, "-D%.*s",
                    (int)def->symbol.count,
                    def->symbol.items);
        }
    }
}

static void gcc_set_output(string_view_t output, build_context_t *build) {

    string_builder_t opath = {.allocator = build->command_builder.allocator};
    sb_append_sv(&opath, build->output_directory);
    if (!sv_ends_with_char(sb_view(&opath), '/')) {
        sb_append_char(&opath, '/');
    }
    sb_append_sv(&opath, output);

    cmd_append(&build->command_builder, "-o");
    cmd_append_sv(&build->command_builder, sb_view(&opath));

    sb_destroy(&opath);
}

static void gcc_set_inputs(const rule_t *rule, build_context_t *build) {
    for (size_t i = 0; i < rule->input.count; ++i) {
        cmd_append_sv(&build->command_builder, sb_view(&rule->input.items[i]));
    }
}

static command_t *compile_gcc(const rule_t *rule, build_context_t *build, void *arg) {
    (void)arg;
    
    cmd_reset(&build->command_builder);
    switch (rule->type)
    {
    case RULE_OBJECT:
        cmd_clone(&build->command_builder, &gcc_compiler.cc);
        cmd_append(&build->command_builder, "-c");
        break;
    case RULE_EXECUTABLE:
        cmd_clone(&build->command_builder, &gcc_compiler.cc);
        break;
    case RULE_DYNAMIC_LIBRARY:
        cmd_clone(&build->command_builder, &gcc_compiler.cc);
        cmd_append(&build->command_builder, "-dynamic"); 
        break;
    case RULE_STATIC_LIBRARY:
        cmd_clone(&build->command_builder, &gcc_compiler.ar);
        cmd_append_sv(&build->command_builder, sb_view(&rule->output));
        for (size_t i = 0; i < rule->input.count; ++i) {
            cmd_append_sv(&build->command_builder, sb_view(&rule->input.items[i]));
        }
        return cmd_exec_async(&build->command_builder);
    } 

    gcc_apply_options(rule, build);
    gcc_set_output(sb_view(&rule->output), build);
    gcc_set_inputs(rule, build); 

    return cmd_exec_async(&build->command_builder);
} 

static void locate_gcc(void) {
   
    env_t env = {.allocator = auto_free_allocator(malloc_allocator)};

    env_inherit_environ(&env);

    gcc_compiler.cc.allocator = arena_allocator(4096, gcc_compiler.buffer);
    gcc_compiler.ar.allocator = gcc_compiler.cc.allocator;

    gcc_compiler.cc.env = &env;
    gcc_compiler.ar.env = &env;

    if (!cmd_find_executable(&gcc_compiler.cc, "gcc")) {
        fprintf(stderr, "Failed to find a gcc executable.\n");
        exit(1);
    }
    if (!cmd_find_executable(&gcc_compiler.ar, "ar")) {
        fprintf(stderr, "Failed to find an archiver.\n");
        exit(1);
    }

    auto_free_destroy(env.allocator);
    gcc_compiler.cc.env = NULL;
    gcc_compiler.ar.env = NULL;
}

compiler_t compiler_gcc(void) {
    
    if (!gcc_compiler.found_gcc) {
        locate_gcc();
        gcc_compiler.found_gcc = true;
    }

    compiler_t compiler = {0};
    compiler.callback = compile_gcc;
    return compiler;
}

