#include <env.h>
#include <allocator.h>
#include <command_line.h>
#include <build-system/project.h>
#include <build-system/rule.h>

_Noreturn
void usage(const char *binary) {
    fprintf(stderr,
        "Usage:\n"
        "  %s\n"
        "    --help          : Print this message and exit.\n"
        "    --self          : Rebuild the project file.\n"
        "    --test          : Build and run all tests.\n"
        "    --echo          : Echo build commands.\n"
        "    --list | -l     : List available targets.\n"
        "    --dry-run       : Print commands that would have been run, and then exit.\n"
        "    --dump-rules    : Print the build rules that would have been run, and then exit.\n"
        "    --target <name> : Choose a build target to compile and link.\n", binary);
    exit(0);
}

int main(int argc, char **argv) {
    
    const char *binary = argv[0];

    if (argc == 1) {
        usage(binary);
    }

    env_t env = {.allocator = malloc_allocator};
    env_inherit_environ(&env);

    bool list = false;
    bool dump_rules = false;
    string_view_t target = SV("cobweb");
    while (argc > 0) {
        if (cl_switch("--help", &argc, &argv) || cl_switch("-h", &argc, &argv)) {
            usage(binary); 
        } else if (cl_switch("--list", &argc, &argv) || cl_switch("-l", &argc, &argv)) {
            list = true;
            break;
        } else if (cl_switch("--echo", &argc, &argv)) {
            env_define(&env, SV("ECHO_BUILD"), SV("1"));
        } else if (cl_switch("--self", &argc, &argv)) {
            target = SV("build");
        } else if (cl_switch("--test", &argc, &argv)) {
            fprintf(stderr, "TODO: Tests are not implemented yet.\n");
            exit(2);
        } else if (cl_switch("--dry-run", &argc, &argv)) {
            env_define(&env, SV("DRY"), SV("1"));
        } else if (cl_switch("--dump-rules", &argc, &argv)) {
            dump_rules = true;
        } else if (cl_arg("--target", &target, &argc, &argv)) {
            continue;
        } else {
            cl_shift(&argc, &argv);
        }
    }

    build_project_t project = {0};
    if (!build_project_load(&project, "recipe.json")) {
        exit(1);
    }

    project_generator_context_t context = project_generator_context(malloc_allocator);
    build_target_t *build_target = query_build_target(target, &project, &context);
    if (!build_target) {
        fprintf(stderr, "Target %.*s does not exist.\n", (int)target.count, target.items);
        exit(2);
    }

    rule_list_t rules = {.allocator = malloc_allocator};
    unsigned int target_index = build_target_emit_rules(build_target, &project, &rules, &context);
    if (target_index == 0) {
        exit(1);
    }

    if (dump_rules) {
        
        json_pool_t pool = {.allocator = malloc_allocator};
        json_pretty_print_with_ansi_highlighting(rule_list_to_json(&rules, &pool), 2, 1, stdout);
        putchar('\n');
        exit(0);
    }

    if (list) {
        for (size_t i = 0; i < project.targets.count; ++i) {
            const build_target_t *target = &project.targets.items[i];
            fprintf(stderr, "%.*s\n", (int)target->name.count, target->name.items);
        }
        exit(0);
    }

    build_context_t build = {0};
    build_context_defaults(&build, &env, malloc_allocator); 
    if (!run_rule(target_index, &rules, &build)) {
        exit(1);
    }
    return 0;
}

