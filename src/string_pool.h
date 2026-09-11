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
 * @file string_pool.h
 *
 * Defines a trie-based data structure for storing global string allocations.
 */

#if !defined(STRING_POOL_H_)
#define STRING_POOL_H_

#include "allocator.h"
#include "string_view.h"

typedef struct string_pool_node string_pool_node_t;
typedef struct string_pool string_pool_t;

struct string_pool_node {
  string_view_t value;
  string_pool_node_t *children[256];
};

struct string_pool {
  allocator_t *allocator;
  string_pool_node_t *root;
};

/**
 * Fetch an existing string from the string pool, or allocate a new string.
 *
 * @param pool String pool to extract a string from.
 * @param[inout] string Pointer to a string_view that holds the string to look
 *                      for, and the variable the located string will be written
 *                      to.
 *
 * @return true on success, false if an allocation error occured.
 */
bool string_pool_extract(string_pool_t *pool, string_view_t *string);

#endif

#if defined(STRING_POOL_IMPLEMENTATION)

#include <string.h>

static bool string_pool_allocate(string_pool_t *pool, string_pool_node_t *node,
                                 size_t pos, string_view_t *string) {

  void *memory = allocator_new(
      pool->allocator, sizeof(size_t) + string->count + 1, _Alignof(size_t));
  if (NULL == memory) {
    return false;
  }
  strncpy((char *)memory + sizeof(size_t), string->items, string->count);
  *(size_t *)memory = string->count;

  string_view_t view = {
      .items = (char *)memory + sizeof(size_t),
      .count = string->count,
  };

  for (size_t i = pos; i < string->count; ++i) {

    string_pool_node_t *child_node =
        allocator_new(pool->allocator, sizeof(string_pool_node_t),
                      _Alignof(string_pool_node_t));
    if (NULL == child_node) {
      allocator_release(pool->allocator, memory,
                        sizeof(size_t) + string->count + 1, _Alignof(size_t));
      return false;
    }
    memset(child_node, 0, sizeof *child_node);

    size_t key = (unsigned char)string->items[i];
    node->children[key] = child_node;
    child_node->value = (string_view_t){
        .items = view.items + i,
        .count = view.count - i,
    };
  }

  *string = view;
  return true;
}

bool string_pool_extract(string_pool_t *pool, string_view_t *string) {

  if (!pool || !string) {
    return false;
  }

  if (string->count == 0) {
    return true;
  }

  if (NULL == pool->root) {
    pool->root = allocator_new(pool->allocator, sizeof(string_pool_node_t),
                               _Alignof(string_pool_node_t));
    if (NULL == pool->root) {
      return false;
    }
    memset(pool->root, 0, sizeof *pool->root);
  }

  string_pool_node_t *node = pool->root;
  if (NULL == node->children[(unsigned char)string->items[0]]) {
    return string_pool_allocate(pool, node, 0, string);
  } else {
    node = node->children[(unsigned char)string->items[0]];
  }

  for (size_t i = 1; i < string->count; ++i) {

    size_t index = (unsigned char)string->items[i];
    if (!node->children[index]) {
      return string_pool_allocate(pool, node, i, string);
    }
    node = node->children[index];
  }

  *string = (string_view_t){
      .items = node->value.items - string->count,
      .count = string->count,
  };
  return true;
}

#endif
