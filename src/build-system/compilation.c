#include "compilation.h"
#include <stddef.h>

struct gcc_compiler {
    bool found_gcc;
    command_builder_t cc;
    command_builder_t ar;

    _Alignas(max_align_t) char buffer[4096];
} gcc_compiler;

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

    if (env_find(build->env, SV("debug"))) {
        cmd_append(&build->command_builder, "-ggdb");
    }

    for (size_t i = 0; i < rule->linker.directories.count; ++i) {
        cmd_format(&build->command_builder, "-L%.*s",
                (int)rule->linker.directories.items[i].count,
                rule->linker.directories.items[i].items);
    }

    for (size_t i = 0; i < rule->linker.libraries.count; ++i) {
        cmd_format(&build->command_builder, "-l%.*s",
                (int)rule->linker.libraries.items[i].count,
                rule->linker.libraries.items[i].items);
    }

    if (rule->linker.rpath.count != 0) {
        cmd_format(&build->command_builder, "-rpath=%.*s",
                (int)rule->linker.rpath.count,
                rule->linker.rpath.items);
    }

    for (size_t i = 0; i < rule->preprocessor.includes.count; ++i) {
        cmd_format(&build->command_builder, "-I%.*s",
                (int)rule->preprocessor.includes.items[i].count,
                rule->preprocessor.includes.items[i].items);
    }

    for (size_t i = 0; i < rule->preprocessor.definitions.count; ++i) {
        preprocessor_definition_t *def = &rule->preprocessor.definitions.items[i];
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

    for (size_t i = 0; i < rule->flags.count; ++i) {
        cmd_append_sv(&build->command_builder, sb_view(&rule->flags.items[i]));
    }
    
    {
        string_builder_t opath = {.allocator = build->command_builder.allocator};
        sb_append_sv(&opath, build->output_directory);
        if (!sv_ends_with_char(sb_view(&opath), '/')) {
            sb_append_char(&opath, '/');
        }
        sb_append_sv(&opath, sb_view(&rule->output));

        cmd_append(&build->command_builder, "-o");
        cmd_append_sv(&build->command_builder, sb_view(&opath));

        sb_destroy(&opath);
    }

    for (size_t i = 0; i < rule->input.count; ++i) {
        cmd_append_sv(&build->command_builder, sb_view(&rule->input.items[i]));
    }

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

