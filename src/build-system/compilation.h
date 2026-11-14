#if !defined(BUILD_SYSTEM_COMPILATION_H_)

#include <stdio.h>

#include "../string_view.h"
#include "../env.h"
#include "../allocator.h"
#include "../command_builder.h"

typedef struct build_recipe build_recipe_t;
typedef struct build_target build_target_t;
typedef struct build_pattern build_pattern_t;

/**
 * Create a new build recipe
 */
build_recipe_t *recipe_create(allocator_t *allocator);

/**
 * Destroy a build recipe and all created targets
 */
void recipe_destroy(build_recipe_t *recipe);

/**
 * Read a recipe from a JSON file
 */
void recipe_read(build_recipe_t *recipe, FILE *f);

/**
 * Inherit all properties from another recipe
 */
void recipe_inherit(build_recipe_t *recipe, build_recipe_t *inherit);

/**
 * Gets the customizable recipe environment
 */
env_t *recipe_env(build_recipe_t *recipe);

/**
 * Adds a build target to the given recipe
 */
build_target_t *recipe_add(build_recipe_t *recipe, string_view_t name);

/**
 * Gets a build target that belongs to this recipe (given a target name)
 */
build_target_t *recipe_get(build_recipe_t *recipe, string_view_t name);

/**
 * Run a build recipe and return a command that can be waited on
 */
command_t *recipe_run(build_recipe_t *recipe);

/**
 * Add a source file to a recipe target
 */
void target_add(build_target_t *target, string_view_t filename);

/**
 * Override the name of the artifact that is built by this target
 */
void target_artifact(build_target_t *target, string_view_t filename);

/**
 * Assigns a build pattern to run for every source file of the target
 */
void target_compiler(build_target_t *target, build_pattern_t *pattern);

/**
 * Assigns a build pattern to run on all collected outputs of the target
 */
void target_linker(build_target_t *target, build_pattern_t *pattern);

/**
 * Gets a customizable recipe environment
 */
env_t *target_env(build_target_t *target);

/**
 * Attaches a build dependency to a target, ensuring the attached dependency
 * always runs first in the build sequence.
 */
void target_depends(build_target_t *target, build_target_t *depends);

#define BUILD_SYSTEM_COMPILATION_H_
#endif
