#if !defined(ENV_H_)
#define ENV_H_

#include "allocator.h"
#include "stream.h"
#include "string_view.h"

typedef struct env env_t;
typedef struct env_node env_node_t;

/**
 * Holds a string-based environment that supports string expansion using $...,
 * ${...} and $(...).
 *
 * $VAR, ${VAR} - Expand the contents of an environment variable.
 *
 * $(VAR args...) - Define $< using the argument after the variable VAR
 *                  (skipping the initial space) and expand the contents
 *                  of VAR.
 *
 */
struct env {
  allocator_t *allocator;
  size_t count;
  env_node_t *root;
};

struct env_node {
  string_builder_t value;
  size_t child_count;
  env_node_t *children[256];
};

#define TYPE_0 env_t
#define TYPE_1 env_node_t
#define TYPE_2 string_builder_t
#define PREFIX env_
#define C_HEADER
#include "trie.h"

/**
 * Allocate a new environment.
 *
 * @param allocator Allocator to use when allocating environment memory.
 *
 * @return The new environment, or NULL on an allocation error.
 */
env_t *env_create(allocator_t *allocator);

/**
 * Release all resources held by an environment.
 */
void env_destroy(env_t *env);

/**
 * Define a new environment variable.
 *
 * @param env Environment to modify.
 * @param key Key to associate with the variable.
 * @param value Value to assign to the variable.
 *
 * @return true on success, false on an allocation error.
 */
bool env_define(env_t *env, string_view_t key, string_view_t value);

/**
 * Extracts a variable from the environment.
 *
 * @param env Environment to fetch a value from.
 * @param name_pattern A pattern to expand into a variable name.
 * @param[out] out_value Pointer to a variable that receives the variable
 *                       definition.
 *
 * @return true if the variable was found, false if it did not exist in the
 *         environment.
 */
bool env_get(env_t *env, string_view_t name_pattern, string_view_t *out_value);

bool env_exec(env_t *env, string_view_t pattern, ostream_t out);

/**
 * Evaluates a pattern inside the environment.
 *
 * @param env Environment to use.
 * @param pattern Pattern to evaluate.
 * @param out Output stream to put the result to.
 *
 * @return true on success, false if the output stream raised an error.
 */
bool env_expand(env_t *env, string_view_t pattern, ostream_t out);

#endif
