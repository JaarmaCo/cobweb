#include <stdio.h>
#include <stdlib.h>

#include <build-system/rule.h>
#include <build-system/compilation.h>

#define ESC "\x1B"
#define BOLD ESC "[1m"
#define RED ESC "[31m"
#define GREEN ESC "[32m"
#define BLUE ESC "[34m"
#define RST ESC "[0m"

#define ERR(str) BOLD RED "[ERROR]: " RST str "\n"
#define OK(str) BOLD GREEN "[OK]: " RST str "\n"
#define INFO(str) BOLD BLUE str RST "\n"

void parsing_rules(allocator_t *allocator) {
    fprintf(stderr, INFO("--- Test parsing_rules"));

    rule_list_t rules = {0};
    json_pool_t pool = {.allocator = allocator};

    FILE *f = fopen("./test/assets/recipe-list.json", "r");
    if (!f) {
        fprintf(stderr, ERR("Failed to open test asset file."));
        exit(1);
    }

    json_node_t json = json_load(&pool, f);
    fclose(f);

    if (!rule_list_from_json(&rules, json, stdout, allocator)) {
        fprintf(stderr, ERR("Trusted test asset failed to parse."));
        exit(1);
    }

    for (size_t i = 0; i < rules.count; ++i) {
        fprintf(stderr, INFO("<< Rule %zu >>"), i + 1);
        
        rule_t *rule = &rules.items[i];
        fprintf(stderr, "output: %.*s\n", (int)rule->output.count, rule->output.items);
        
        fprintf(stderr, "input:\n");
        for (size_t i = 0; i < rule->input.count; ++i) {
            fprintf(stderr, "    - %.*s\n",
                    (int)rule->input.items[i].count,
                    rule->input.items[i].items);
        }

        switch (rule->type)
        {
        case RULE_OBJECT:
            fprintf(stderr, "type: object\n");
            break;
        case RULE_DYNAMIC_LIBRARY:
            fprintf(stderr, "type: dynamic-library\n");
            break;
        case RULE_STATIC_LIBRARY:
            fprintf(stderr, "type: static-library\n");
            break;
        case RULE_EXECUTABLE:
            fprintf(stderr, "type: executable\n");
            break;
        }

        if (rule->depends.count != 0) {
            fprintf(stderr, "depends:\n");
            for (size_t i = 0; i < rule->depends.count; ++i) {
                fprintf(stderr, "    - %u\n", rule->depends.items[i]);
            }
        }

        if (rule->flags.count != 0) {
            fprintf(stderr, "flags:\n");
            for (size_t i = 0; i < rule->flags.count; ++i) {
                fprintf(stderr, "    - %.*s\n",
                        (int)rule->flags.items[i].count, rule->flags.items[i].items);
            }
        }

        if (rule->linker.rpath.count != 0
                || rule->linker.directories.count != 0
                || rule->linker.libraries.count != 0) {
            fprintf(stderr, "linker:\n");
            if (rule->linker.rpath.count != 0) {
                fprintf(stderr, "    rpath: %.*s\n",
                        (int)rule->linker.rpath.count, rule->linker.rpath.items);
            }
            if (rule->linker.directories.count != 0) {
                fprintf(stderr, "    directories:\n");
                for (size_t i = 0; i < rule->linker.directories.count; ++i) {
                    fprintf(stderr, "        - %.*s\n",
                            (int)rule->linker.directories.items[i].count,
                            rule->linker.directories.items[i].items);
                }
            }
            if (rule->linker.libraries.count != 0) {
                fprintf(stderr, "    libraries:\n");
                for (size_t i = 0; i < rule->linker.libraries.count; ++i) {
                    fprintf(stderr, "        - %.*s\n",
                            (int)rule->linker.libraries.items[i].count,
                            rule->linker.libraries.items[i].items);
                }
            }
        }

        if (rule->preprocessor.definitions.count != 0
                || rule->preprocessor.includes.count != 0) {
            fprintf(stderr, "preprocessor:\n");
            if (rule->preprocessor.includes.count != 0) {
                fprintf(stderr, "    includes:\n");
                for (size_t i = 0; i < rule->preprocessor.includes.count; ++i) {
                    fprintf(stderr, "        - %.*s\n",
                            (int)rule->preprocessor.includes.items[i].count,
                            rule->preprocessor.includes.items[i].items);
                }
            }
            if (rule->preprocessor.definitions.count != 0) {
                fprintf(stderr, "    definitions:\n");
                for (size_t i = 0; i < rule->preprocessor.definitions.count; ++i) {
                    string_view_t view = rule->preprocessor.definitions.items[i].expansion.count == 0
                                ? SV("null")
                                : sb_view(&rule->preprocessor.definitions.items[i].expansion);
                    fprintf(stderr, "        %.*s: %.*s\n",
                            (int)rule->preprocessor.definitions.items[i].symbol.count,
                            rule->preprocessor.definitions.items[i].symbol.items,
                            (int)view.count,
                            view.items);
                }
            }
        }
    }

    json_destroy_pool(&pool);
    rule_list_destroy(&rules);
}

void building(allocator_t *allocator) {
    fprintf(stderr, INFO("--- Test building"));

    rule_list_t rules = {0};
    json_pool_t pool = {.allocator = allocator};

    FILE *f = fopen("./test/assets/recipe-list.json", "r");
    if (!f) {
        fprintf(stderr, ERR("Failed to open test asset file."));
        exit(1);
    }

    json_node_t json = json_load(&pool, f);
    fclose(f);

    if (!rule_list_from_json(&rules, json, stdout, allocator)) {
        fprintf(stderr, ERR("Trusted test asset failed to parse."));
        exit(1);
    }
    
    allocator_t *auto_free_context = auto_free_allocator(allocator);
    env_t env = {.allocator = auto_free_context};

    build_context_t context;
    build_context_defaults(&context, &env, auto_free_context);

    context.output_directory = SV("test-out");

    if (!run_rule(1, &rules, &context)) {
        fprintf(stderr, ERR("Failed to run the rule."));
        exit(1);
    }

    auto_free_destroy(auto_free_context);
    json_destroy_pool(&pool);
    rule_list_destroy(&rules);
}

void run_test(void(*test)(allocator_t *)) {
    
    size_t allocated_size = 0;
    debug_allocator_t a =
        trace_free_balance_allocator(malloc_allocator, &allocated_size);

    test(&a.base);

    if (allocated_size != 0) {
        fprintf(stderr, ERR("Leaked %zu bytes"), allocated_size);
        exit(1);
    } 
}

int main(void) {
    run_test(parsing_rules);
    run_test(building);
    return 0;
}

