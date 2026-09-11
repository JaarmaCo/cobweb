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

/**
 * @file env.h
 *
 * This header defines an "environment" (env_t) data type for storing and
 * formatting using key-value pairs of strings.
 *
 * An environment is a trie datastructure that maps strings to strings, and
 * provides a set of functions for shell-like expansion with the environment
 * values.
 *
 * The system is designed to be used with an allocator that can be freed.
 * That is, the default implementation does not provide a destroy function,
 * as memory management is intended to be managed externally. Please take
 * this into account when using env_t.
 */
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
 * Add defines for every entry in the system environment.
 *
 * @param env Environment to modify
 *
 * @return true on success.
 */
bool env_inherit_environ(env_t *env);

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

/**
 * Expands a pattern using execution rules. That is, it expands the pattern,
 * and splits the evaluated string into two parts near the first space or
 * non-variable name character.
 *
 * The contents of the variable the left hand string in the split points to is
 * then evaluated with $< equal to the right hand side of the split.
 *
 * Example:
 *  foo=value is $<
 *  foo bar
 *
 * Expands to:
 *  value is bar
 *
 * @param env Environment to use in the expansion.
 * @param pattern The pattern to expand and "execute".
 * @param out Stream to collect the output into.
 *
 * @return true on success, false if an I/O or allocation error occured.
 */
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

/**
 * Dumps the contents of the environment to the given output stream.
 *
 * @param env Environment to dump.
 *
 * @return true on success, false if an I/O error occured.
 */
bool env_dump(env_t *env, ostream_t out);

#if defined(UNITY_BUILD)
#include "env.c"
#endif
#endif
