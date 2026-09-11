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
 * @file trie.h
 *
 * This file is a template header file that defines a trie data structure for
 * associating string keys with values (see https://en.wikipedia.org/wki/Trie).
 *
 * The following macros are available for definition at template instantiation:
 *
 * - TYPE_0 : Defines the trie struct type (optional)
 * - TYPE_1 : Defines the trie node struct type (optional)
 * - TYPE_2 : Defines the stored value type
 * - TRIE_MIN_CHAR : Minimum character value that can be stored in the trie
 *                   (optional, default is 0)
 * - TRIE_MAX_CHAR : Maximum character value that can be stored in the trie
 *                   (optional, default is 255)
 */

#if !defined(HEADER_ONLY) && !defined(C_HEADER) && !defined(C_SOURCE)
#define TYPE_2 int
#define HEADER_ONLY
#endif

#define REQUIRE_TYPE_2
#include "template-def.h"

#if defined(C_HEADER)

#include "allocator.h"
#include "string_view.h"

#if !defined(TYPE_0)
#define TYPE_0 M_CAT(trie, SUFFIX)
#endif

#if !defined(TYPE_1)
#define TYPE_1 M_CAT(trie_node, SUFFIX)
#endif

#define trie_t TYPE_0
#define trie_node_t TYPE_1
#define T TYPE_2

#define trie_find M_NAME(find)
#define trie_insert M_NAME(insert)
#define trie_delete M_NAME(delete)

#if !defined(TRIE_MIN_CHAR)
#define TRIE_MIN_CHAR 0
#endif

#if !defined(TRIE_MAX_CHAR)
#define TRIE_MAX_CHAR 255
#endif

#if defined(DEFINE_STRUCT)

typedef struct trie_t trie_t;
typedef struct trie_node_t trie_node_t;

struct trie_node_t {

  /**
   * Value stored in this node
   */
  T value;

  /**
   * Number of child nodes.
   */
  size_t child_count;

  /**
   * Character-indexed array of child pointers.
   */
  trie_node_t *children[1 + TRIE_MAX_CHAR - TRIE_MIN_CHAR];
};

struct trie_t {

  /**
   * Pointer to the allocator that is used when allocating new nodes.
   */
  allocator_t *allocator;

  /**
   * Number of entries in the trie.
   */
  size_t count;

  /**
   * Pointer to the first node in the trie.
   */
  trie_node_t *root;
};

#endif

/**
 * Look up a node in the trie.
 *
 * @param trie Trie to search.
 * @param key String to search for.
 *
 * @return Either the trie node that matches the provided string key, or NULL.
 */
INLINE
trie_node_t *trie_find(trie_t const *trie, string_view_t key);

/**
 * Inserts a new key-value-pair into the trie.
 *
 * @param trie Trie to insert into.
 * @param key Key of the association.
 * @param value Value to associate with.
 *
 * @return true on success, false if an allocation error occured.
 */
INLINE
bool trie_insert(trie_t *trie, string_view_t key, T value);

/**
 * Deletes a node from the trie.
 *
 * @param trie Trie to delete from.
 * @param key Key of the entry to delete.
 *
 * @return true if the entry was deleted, false if no entry in the trie was
 *         associated with the given key.
 */
INLINE
bool trie_delete(trie_t *trie, string_view_t key);

#endif

#if defined(C_SOURCE)

#include <assert.h>
#include <string.h>

#define trie_new_node M_NAME(new_node)
#define trie_node_delete M_NAME(node_delete)
#define trie_to_index M_NAME(to_index)

INLINE
size_t trie_to_index(char c) {
  unsigned char ch = (unsigned char)c;

#if TRIE_MIN_CHAR == 0 && TRIE_MAX_CHAR == 255
  return (size_t)ch;
#elif TRIE_MIN_CHAR > 0
  assert(ch >= TRIE_MIN_CHAR);
  if (ch < TRIE_MIN_CHAR) {
    return 0;
  }
  return (size_t)ch;
#elif TRIE_MAX_CHAR < 255
  assert(ch <= TRIE_MAX_CHAR);
  if (ch > TRIE_MAX_CHAR) {
    return 0;
  }
  return (size_t)ch;
#else
  assert(ch >= TRIE_MIN_CHAR && ch <= TRIE_MAX_CHAR);
  if (ch < TRIE_MIN_CHAR || ch > TRIE_MAX_CHAR) {
    return 0;
  }
  return ch - TRIE_MIN_CHAR;
#endif
}

INLINE
trie_node_t *trie_new_node(trie_t *trie) {

  trie_node_t *node = allocator_new(trie->allocator, sizeof(trie_node_t),
                                    _Alignof(trie_node_t));
  if (NULL == node) {
    return NULL;
  }
  memset(node, 0, sizeof *node);
  return node;
}

INLINE
trie_node_t *trie_find(trie_t const *trie, string_view_t key) {
  trie_node_t *node = trie->root;
  for (size_t i = 0; i < key.count; ++i) {
    if (NULL == node) {
      return NULL;
    }
    node = node->children[trie_to_index(key.items[i])];
  }
  return node;
}

INLINE
bool trie_insert(trie_t *trie, string_view_t key, T value) {

  if (NULL == trie->root) {
    trie->root = trie_new_node(trie);
    if (NULL == trie->root) {
      return false;
    }
  }

  trie_node_t *node = trie->root;
  for (size_t i = 0; i < key.count; ++i) {

    size_t index = trie_to_index(key.items[i]);
    if (!node->children[index]) {
      node->children[index] = trie_new_node(trie);
      if (!node->children[index]) {
        return false;
      }
      ++node->child_count;
    }
    node = node->children[index];
  }
  ++trie->count;
  node->value = value;
  return true;
}

INLINE
trie_node_t *trie_node_delete(trie_t *trie, trie_node_t *node,
                              string_view_t key, bool *out_match) {

  if (NULL == node) {
    return NULL;
  }

  if (key.count == 0) {
    *out_match = true;
  } else {
    size_t index = trie_to_index(key.items[0]);
    node->children[index] = trie_node_delete(trie, node->children[index],
                                             sv_drop(key, 1), out_match);

    if (*out_match) {
      --node->child_count;
    }
  }
  if (node->child_count == 0) {
    allocator_release(trie->allocator, node, sizeof(trie_node_t),
                      _Alignof(trie_node_t));
    return NULL;
  }
  return node;
}

INLINE
bool trie_delete(trie_t *trie, string_view_t key) {
  bool match = false;
  trie->root = trie_node_delete(trie, trie->root, key, &match);
  if (match) {
    --trie->count;
  }
  return match;
}
#endif

#undef T
#undef trie_t
#undef trie_node_t
#undef trie_find
#undef trie_delete
#undef trie_insert
#undef trie_new_node
#undef trie_node_delete
#undef TRIE_MIN_CHAR
#undef TRIE_MAX_CHAR

#include "template-undef.h"
