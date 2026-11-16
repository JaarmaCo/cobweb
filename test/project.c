#include <build-system/project.h>

#define ESC "\x1B"
#define BOLD ESC "[1m"
#define RED ESC "[31m"
#define GREEN ESC "[32m"
#define BLUE ESC "[34m"
#define RST ESC "[0m"

#define ERR(str) BOLD RED "[ERROR]: " RST str "\n"
#define OK(str) BOLD GREEN "[OK]: " RST str "\n"
#define INFO(str) BOLD BLUE str RST "\n"

void parsing(allocator_t *allocator) {
    
    fprintf(stderr, INFO("--- Test parsing"));

    build_project_t project = {0};
    json_pool_t pool = {.allocator = allocator};

    FILE *f = fopen("test/assets/recipe.json", "r");
    if (!f) {
        fprintf(stderr, ERR("Failed to open asset file."));
        exit(1);
    }

    json_node_t project_json = json_load(&pool, f);
    fclose(f);

    if (!build_project_from_json(&project, project_json, stderr, allocator)) {
        fprintf(stderr, ERR("Failed to load project descriptor."));
        exit(1);
    }

    json_node_t serialized = build_project_to_json(&project, &pool);
    
    cmd_ensure_directory("test-out");
    FILE *out = fopen("test-out/project-out.json", "w");
    if (!out) {
        fprintf(stderr, ERR("Failed to open output file."));
        exit(1);
    }

    json_pretty_print(serialized, 2, 1, out);
    fputc('\n', out);
    fclose(out);
    fprintf(stderr, INFO("Wrote: test-out/project-out.json"));

    json_destroy_pool(&pool);
    build_project_destroy(&project);
}

void emitting(allocator_t *allocator) {

    fprintf(stderr, INFO("--- Test emitting"));

    build_project_t project = {0};
    json_pool_t pool = {.allocator = allocator};

    FILE *f = fopen("test/assets/recipe.json", "r");
    if (!f) {
        fprintf(stderr, ERR("Failed to open asset file."));
        exit(1);
    }

    json_node_t project_json = json_load(&pool, f);
    fclose(f);

    if (!build_project_from_json(&project, project_json, stderr, allocator)) {
        fprintf(stderr, ERR("Failed to load project descriptor."));
        exit(1);
    }

    rule_list_t rules = {.allocator = allocator};
    project_generator_context_t context = project_generator_context(allocator);

    if (!build_project_emit_rules(&project, &rules, &context)) {
        fprintf(stderr, ERR("Failed to emit project rules."));
        exit(1);
    }

    cmd_ensure_directory("test-out");
    
    FILE *out = fopen("test-out/project-rules-out.json", "w");
    if (!out) {
        fprintf(stderr, ERR("Failed to open output file."));
        exit(1);
    }

    json_pretty_print(rule_list_to_json(&rules, &pool), 2, 1, out);
    fputc('\n', out);
    fclose(out);
    fprintf(stderr, INFO("Wrote: test-out/project-rules-out.json"));

    out = fopen("test-out/project-dependency-graph.json", "w");
    if (!out) {
        fprintf(stderr, ERR("Failed to open output file."));
        exit(1);
    }

    json_pretty_print(dependency_link_map_to_json(&context.links, &pool), 2, 1, out);
    fputc('\n', out);
    fclose(out);
    fprintf(stderr, INFO("Wrote: test-out/project-dependency-graph.json"));

    project_generator_context_destroy(&context);
    json_destroy_pool(&pool);
    rule_list_destroy(&rules);
    build_project_destroy(&project);
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
    run_test(parsing);
    run_test(emitting);
    return 0;
}

