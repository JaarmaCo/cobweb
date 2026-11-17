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

#ifndef BUILD_SYSTEM_PROJECT_H_

#include "rule.h"

typedef struct build_project build_project_t;
typedef struct build_target build_target_t;
typedef struct build_target_list build_target_list_t;
typedef struct target_compiler_options target_compiler_options_t;
typedef struct project_generator_context project_generator_context_t;
typedef struct target_dependency target_dependency_t;
typedef struct target_dependency_map target_dependency_map_t;
typedef struct dependency_link dependency_link_t;
typedef struct dependency_link_map dependency_link_map_t;

struct target_compiler_options {
    
    /**
     * Arguments that applies to both this
     * target, and inherited targets.
     */
    compiler_options_t shared;
    
    /**
     * Arguments that apply only to this target.
     */
    compiler_options_t internal;

    /**
     * Arguments to pass to dependent target, that
     * don't apply to this target.
     */
    compiler_options_t external;
};

struct build_target {
    
    /**
     * Name of the build target.
     */
    string_builder_t name;

    /**
     * Name of the artifact produced by this target.
     */
    string_builder_t output;

    /**
     * Type of artifact this target produces.
     */
    rule_type_t type;

    /**
     * List of directories to search when locating source files.
     */
    file_list_t source_directories;

    /**
     * The source files this target contains.
     */
    file_list_t sources;

    /**
     * List of directories to search when locating test files.
     */
    file_list_t test_directories;

    /**
     * Directory where output files are placed.
     */
    string_builder_t output_directory;

    /**
     * List of target test files.
     */
    file_list_t tests;

    /**
     * List of target dependency names.
     */
    file_list_t depends;

    /**
     * Options to pass to the different build phases.
     */
    target_compiler_options_t options;
};

struct build_target_list {

    size_t count;
    size_t capacity;
    build_target_t *items;
    allocator_t *allocator;
};

struct build_project {
    
    /**
     * List of source directories to search for source
     * files in every target of this project.
     */
    file_list_t source_directories;

    /**
     * List of source directories to search for test source
     * files in every target of this project.
     */
    file_list_t test_directories;

    /**
     * Global compilation options. 
     */
    compiler_options_t options;

    /**
     * List of targets to build.
     */
    build_target_list_t targets;

    /**
     * Output directory prefix to use if the target did not
     * name an output directory.
     */
    string_builder_t output_directory;
};

struct target_dependency {
    size_t hash;
    string_builder_t key;
    unsigned int value;
};

struct target_dependency_map {
    size_t count;
    size_t capacity;
    target_dependency_t *items;
    allocator_t *allocator;
};

struct dependency_link {
    size_t hash;
    string_builder_t key;
    target_dependency_map_t value;
};

struct dependency_link_map {
    size_t count;
    size_t capacity;
    dependency_link_t *items;
    allocator_t *allocator;
};

struct project_generator_context {
   
    /**
     * File stream to echo status messages to
     */
    FILE *status;

    /**
     * Allocator to use when constructing the rule datastructures.
     */
    allocator_t *allocator;

    /**
     * Implementation detail, set to 0
     */
    size_t dependency_depth;

    /**
     * Cache used for looking up targets by name
     */
    target_dependency_map_t indexed_targets;

    /**
     * Registry of artifacts (by resource locator) and their rule indices
     */
    target_dependency_map_t artifact_rules;

    /**
     * Map of targets that are already added
     */
    target_dependency_map_t completed_targets;

    /**
     * Links names to the transitive dependencies they imply
     */
    dependency_link_map_t links;
};

static inline project_generator_context_t project_generator_context(allocator_t *allocator) {
    project_generator_context_t context = {
        .allocator = allocator,
        .indexed_targets = {
            .allocator = allocator,
        },
        .artifact_rules = {
            .allocator = allocator,
        },
        .completed_targets = {
            .allocator = allocator,
        },
        .links = {
            .allocator = allocator,
        },
        .status = stderr,
        .dependency_depth = 0,
    };
    return context;
}

/**
 * Generate a rule list based on the provided project description.
 */
bool build_project_emit_rules(const build_project_t *project,
        rule_list_t *rules, project_generator_context_t *context);

unsigned int build_target_emit_rules(const build_target_t *target, const build_project_t *project,
        rule_list_t *rules, project_generator_context_t *context);

void target_compiler_options_destroy(target_compiler_options_t *value);

void build_target_destroy(build_target_t *value);

void build_target_list_destroy(build_target_list_t *value);

void build_project_destroy(build_project_t *value);

void project_generator_context_destroy(project_generator_context_t *context);

bool target_dependency_map_from_json(target_dependency_map_t *value,
        json_node_t json, FILE *status, allocator_t *allocator);

bool dependency_link_map_from_json(dependency_link_map_t *value,
        json_node_t json, FILE *status, allocator_t *allocator);

bool target_compiler_options_from_json(target_compiler_options_t *value, 
    json_node_t json, FILE *status, allocator_t *allocator);

bool build_target_from_json(build_target_t *value,
    json_node_t json, FILE *status, allocator_t *allocator);

bool build_target_list_from_json(build_target_list_t *value,
    json_node_t json, FILE *status, allocator_t *allocator);

bool build_project_from_json(build_project_t *value,
    json_node_t json, FILE *status, allocator_t *allocator);

json_node_t target_dependency_map_to_json(const target_dependency_map_t *value, json_pool_t *pool);

json_node_t dependency_link_map_to_json(const dependency_link_map_t *value, json_pool_t *pool);

json_node_t target_compiler_options_to_json(const target_compiler_options_t *value, json_pool_t *pool);

json_node_t build_target_to_json(const build_target_t *value, json_pool_t *pool);

json_node_t build_target_list_to_json(const build_target_list_t *value, json_pool_t *pool);

json_node_t build_project_to_json(const build_project_t *value, json_pool_t *pool);

#define TYPE_0 build_target_list_t
#define TYPE_1 build_target_t
#define FUNCTION_0 build_target_destroy
#define PREFIX build_target_list_
#define SUFFIX 
#define C_HEADER
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
#define C_HEADER
#include "../hashmap.h"

#define TYPE_0 dependency_link_map_t
#define TYPE_1 dependency_link_t
#define TYPE_2 string_builder_t
#define TYPE_3 target_dependency_map_t
#define FUNCTION_0(e) do { sb_destroy(&(e)->key); target_dependency_map_destroy(&(e)->value); } while(0)
#define FUNCTION_1(k) sv_hash(sb_view(&k))
#define FUNCTION_2(x, y) sv_equals(sb_view(&x), sb_view(&y))
#define PREFIX dependency_link_map_
#define SUFFIX
#define C_HEADER
#include "../hashmap.h"

#define BUILD_SYSTEM_PROJECT_H_
#endif
