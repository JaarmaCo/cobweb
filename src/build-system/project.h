#ifndef BUILD_SYSTEM_PROJECT_H_

#include "rule.h"

typedef struct build_project build_project_t;
typedef struct build_target build_target_t;
typedef struct build_target_list build_target_list_t;
typedef struct target_compiler_options target_compiler_options_t;

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
};

/**
 * Append the rules required to produce the target of the given name.
 *
 * @param name Name of the target to produce.
 * @param project Project where the target is found.
 * @param rules List of rules to append to.
 * @param context Context of the build process.
 *
 * @return true on success, false in the case of an error.
 */
bool target_emit_rules(string_view_t name, const build_project_t *project,
        rule_list_t *rules, build_context_t *context);

#define BUILD_SYSTEM_PROJECT_H_
#endif
