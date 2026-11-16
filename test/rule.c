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

    cmd_ensure_directory("test-out");
    FILE *out = fopen("test-out/rule-out.json", "w");
    if (!out) {
        fprintf(stderr, ERR("Failed to open output file."));
        exit(1);
    }

    json_pretty_print(rule_list_to_json(&rules, &pool), 2, 1, out);
    fputc('\n', out);
    fclose(out);

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

