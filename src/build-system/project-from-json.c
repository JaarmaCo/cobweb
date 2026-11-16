#include "json.h"
#include "project.h"

bool target_dependency_map_from_json(target_dependency_map_t *value,
        json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_STRING_MAP_FROM_JSON_IMPLEMENTATION(target_dependency_map_t,
            string_builder_t, unsigned int, dependency_index,
            sb_append_sv, target_dependency_map_insert);
}

bool dependency_link_map_from_json(dependency_link_map_t *value,
        json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_STRING_MAP_FROM_JSON_IMPLEMENTATION(dependency_link_map_t,
            string_builder_t, target_dependency_map_t, target_dependency_map,
            sb_append_sv, dependency_link_map_insert);
}

bool target_compiler_options_from_json(target_compiler_options_t *value, 
    json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_OBJECT_FROM_JSON_IMPLEMENTATION(target_compiler_options_t,
        (compiler_options, shared, "shared", T),
        (compiler_options, internal, "internal", T),
        (compiler_options, external, "external", T));
}

bool build_target_from_json(build_target_t *value,
    json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_OBJECT_FROM_JSON_IMPLEMENTATION(built_target_t,
        (string_builder, name, "name", F),
        (string_builder, output, "output", T),
        (rule_type, type, "type", T),
        (file_list, source_directories, "source-directories", T),
        (file_list, sources, "sources", T),
        (file_list, test_directories, "test-directories", T),
        (file_list, tests, "tests", T),
        (file_list, depends, "depends", T),
        (target_compiler_options, options, "options", T));
}

bool build_target_list_from_json(build_target_list_t *value,
    json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_LIST_FROM_JSON_IMPLEMENTATION(build_target_list_t,
            build_target_t, build_target, build_target_list_append);
}

bool build_project_from_json(build_project_t *value,
    json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_OBJECT_FROM_JSON_IMPLEMENTATION(build_project_t,
            (file_list, source_directories, "source-directories", T),
            (file_list, test_directories, "test-directories", T),
            (compiler_options, options, "options", T),
            (build_target_list, targets, "targets", T));
}

json_node_t target_dependency_map_to_json(const target_dependency_map_t *value, json_pool_t *pool) {
    JSON_STRING_MAP_TO_JSON_IMPLEMENTATION(dependency_index);
}

json_node_t dependency_link_map_to_json(const dependency_link_map_t *value, json_pool_t *pool) {
    JSON_STRING_MAP_TO_JSON_IMPLEMENTATION(target_dependency_map);
}

json_node_t target_compiler_options_to_json(const target_compiler_options_t *value, json_pool_t *pool) {
    JSON_OBJECT_TO_JSON_IMPLEMENTATION(
        (compiler_options, shared, "shared", T),
        (compiler_options, internal, "internal", T),
        (compiler_options, external, "external", T));
}

json_node_t build_target_to_json(const build_target_t *value, json_pool_t *pool) {
    JSON_OBJECT_TO_JSON_IMPLEMENTATION(
        (string_builder, name, "name", F),
        (string_builder, output, "output", T),
        (rule_type, type, "type", T),
        (file_list, source_directories, "source-directories", T),
        (file_list, sources, "sources", T),
        (file_list, test_directories, "test-directories", T),
        (file_list, tests, "tests", T),
        (file_list, depends, "depends", T),
        (target_compiler_options, options, "options", T));
}

json_node_t build_target_list_to_json(const build_target_list_t *value, json_pool_t *pool) {
    JSON_LIST_TO_JSON_IMPLEMENTATION(build_target);
}

json_node_t build_project_to_json(const build_project_t *value, json_pool_t *pool) {
    JSON_OBJECT_TO_JSON_IMPLEMENTATION(
            (file_list, source_directories, "source-directories", T),
            (file_list, test_directories, "test-directories", T),
            (compiler_options, options, "options", T),
            (build_target_list, targets, "targets", T));
}


