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
  T value;
  size_t child_count;
  trie_node_t *children[1 + TRIE_MAX_CHAR - TRIE_MIN_CHAR];
};

struct trie_t {
  allocator_t *allocator;
  size_t count;
  trie_node_t *root;
};

#endif

INLINE
trie_node_t *trie_find(trie_t const *trie, string_view_t key);

INLINE
bool trie_insert(trie_t *trie, string_view_t key, T value);

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
trie_node_t *trie_node_delete(trie_node_t *node, string_view_t key,
                              bool *out_match) {

  if (NULL == node) {
    return NULL;
  }

  if (key.count == 0) {
    *out_match = true;
  } else {
    size_t index = trie_to_index(key.items[0]);
    node->children[index] =
        trie_node_delete(node->children[index], sv_drop(key, 1), out_match);

    if (*out_match) {
      --node->child_count;
    }
  }
  return node->child_count == 0 ? NULL : node;
}

INLINE
bool trie_delete(trie_t *trie, string_view_t key) {
  bool match = false;
  trie->root = trie_node_delete(trie->root, key, &match);
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
