// clang-format off
//
// hashmap.h
//
// This is a "templated" C header file containing a flat hashmap implementation
// that permits parameterization using user defined macros.
//
// The parameters for this file are:
//
// - #define TYPE_0 // The hashmap structure type (optional)
// - #define TYPE_1 // The hashmap entry type (optional)
// - #define TYPE_2 // The key type (required)
// - #define TYPE_3 // The value type (required)
// - #define FUNCTION_0(TYPE_1 *) // Hashmap entry destructor (optional)
// - #define FUNCTION_1(TYPE_2) // Key type hash function (required)
// - #define FUNCTION_2(TYPE_3) // Key type equals function (required)
// - #define PREFIX // Prefix to prepend to all generated function (optional)
// - #define SUFFIX // Suffix to append to all generated symbols (optional)
// - #define C_HEADER // If defined, produces an (unguarded) C-header file
// - #define C_SOURCE // If defined, produces a C source file
// - #define HEADER_ONLY // If defined, inlines all functions and includes both source and header
//
// clang-format on

#if !defined(C_SOURCE) && !defined(C_HEADER) && !defined(HEADER_ONLY)
#define TYPE_2 int
#define TYPE_3 int
#define FUNCTION_1 hash_int
#define FUNCTION_2(x, y) ((x) == (y))
#define HEADER_ONLY
#define SUFFIX _i
#include "hash.h"
#endif

#define REQUIRE_TYPE_2
#define REQUIRE_TYPE_3
#define REQUIRE_FUNCTION_1
#define REQUIRE_FUNCTION_2
#include "template-def.h"

#if defined(C_HEADER)

#include <stdbool.h>
#include <stdint.h>

#include "allocator.h"

#if !defined(TYPE_0)
#define TYPE_0 M_CAT(hashmap, SUFFIX)
#endif

#if !defined(TYPE_1)
#define TYPE_1 M_CAT(hashmap_entry, SUFFIX)
#endif

#define TKey TYPE_2
#define TValue TYPE_3

#define hash_key FUNCTION_1
#define equals_key FUNCTION_2

#define hashmap TYPE_0
#define hashmap_entry TYPE_1
#define insert M_NAME(insert)
#define replace M_NAME(replace)
#define put M_NAME(put)
#define remove M_NAME(remove)
#define delete_ M_NAME(delete)
#define find M_NAME(find)
#define destroy M_NAME(destroy)
#define grow M_NAME(grow)
#define destroy M_NAME(destroy)

#if defined(DEFINE_STRUCT)

typedef struct hashmap_entry {
  TKey key;
  TValue value;
  size_t hash;
} hashmap_entry;

typedef struct hashmap {
  hashmap_entry *items;
  size_t count;
  size_t capacity;
  allocator_t *allocator;
} hashmap;

#endif

/**
 * Free all memory owned by the given hashmap.
 */
INLINE
void destroy(hashmap *hm);

/**
 * Insert an entry into the hashmap.
 *
 * @param hm Hasmap to modify.
 * @param key Key of the entry to insert.
 * @param value Value to associate with the entry.
 *
 * @return A pointer to the entry, or NULL if the entry existed or on an
 *         allocation failure.
 */
INLINE
hashmap_entry *insert(hashmap *hm, TKey key, TValue value);

/**
 * Replace an existing entry with a new value.
 *
 * @param hm Hashmap to modify.
 * @param key Key of the value that is to be replaced.
 * @param value New value to set the entry to.
 *
 * @return A pointer to the modified entry, or NULL if the entry did not exist.
 */
INLINE
hashmap_entry *replace(hashmap *hm, TKey key, TValue value);

/**
 * Insert or replace an existing entry.
 *
 * @param hm Hashmap to modify.
 * @param key Key to associate with the given value.
 * @param value Value to add or replace with.
 *
 * @return A pointer to the new/modified entry, or NULL on allocation failure.
 */
INLINE
hashmap_entry *put(hashmap *hm, TKey key, TValue value);

/**
 * Finds an existing entry in the hashmap.
 *
 * @param hm Hashmap to seek through.
 * @param key Key to look for.
 *
 * @return A pointer to the located entry, or NULL if no entry was found.
 */
INLINE
hashmap_entry *find(const hashmap *hm, TKey key);

/**
 * Remove an existing entry from the hashmap.
 *
 * @param hm Hashmap to remove an entry from.
 * @param entry Pointer to the entry to remove.
 */
INLINE
void remove(hashmap *hm, hashmap_entry *entry);

/**
 * Delete an entry from the hashmap.
 *
 * @param hm Hashmap to modify.
 * @param key Key of the element to delete.
 *
 * @return true if the element was deleted, false if no such element exists in
 *         the map.
 */
INLINE
bool delete_(hashmap *hm, TKey key);

#endif

#if defined(C_SOURCE)

#include <assert.h>
#include <string.h>

INLINE
void destroy(hashmap *hm) {
  if (!hm || !hm->items) {
    return;
  }

#if defined(FUNCTION_0)
  for (size_t i = 0; i < hm->capacity; ++i) {
    if (hm->items[i].hash != 0) {
      FUNCTION_0(&hm->items[i]);
    }
  }
#endif

  allocator_release(hm->allocator, hm->items,
                    hm->capacity * sizeof(hashmap_entry),
                    _Alignof(hashmap_entry));
  memset(hm, 0, sizeof *hm);
}

INLINE
bool grow(hashmap *hm) {
  const size_t DEFAULT_SIZE = 4;

  size_t new_size = hm->capacity == 0 ? DEFAULT_SIZE : hm->capacity * 2;
  hashmap_entry *new_items = allocator_new(
      hm->allocator, new_size * sizeof(hashmap_entry), _Alignof(hashmap_entry));
  if (NULL == new_items) {
    return false;
  }
  memset(new_items, 0, new_size * sizeof(hashmap_entry));

  for (size_t i = 0; i < hm->capacity; ++i) {

    if (hm->items[i].hash == 0) {
      continue; // Not occupied
    }

    hashmap_entry *entry = &hm->items[i];

    size_t position = entry->hash % new_size;
    for (size_t j = 1; j <= new_size; ++j) {
      if (new_items[position].hash == 0) {
        new_items[position] = *entry;
        break;
      }
      position = (position + j * j) % new_size;
    }
  }

  allocator_release(hm->allocator, hm->items,
                    hm->capacity * sizeof(hashmap_entry),
                    _Alignof(hashmap_entry));
  hm->capacity = new_size;
  hm->items = new_items;
  return true;
}

INLINE
hashmap_entry *find(const hashmap *hm, TKey key) {

  if (hm->count == 0) {
    return NULL;
  }

  size_t hash = hash_key(key);
  hash = hash == 0 ? 1 : hash;

  size_t position = hash % hm->capacity;
  for (size_t i = 1; i <= hm->capacity; ++i) {
    if (hm->items[position].hash == hash &&
        equals_key(key, hm->items[position].key)) {
      return &hm->items[position];
    }
    position = (position + i * i) % hm->capacity;
  }
  return NULL;
}

INLINE
hashmap_entry *insert(hashmap *hm, TKey key, TValue value) {

  if (hm->count + 1 > hm->capacity) {
    if (!grow(hm)) {
      return NULL;
    }
  }

  size_t hash = hash_key(key);
  hash = hash == 0 ? 1 : hash;

  size_t position = hash % hm->capacity;
  for (size_t i = 1; i <= hm->capacity; ++i) {

    if (hm->items[position].hash == 0) {
      ++hm->count;
      break; // Not occupied
    }

    if (equals_key(key, hm->items[position].key)) {
      return NULL;
    }

    position = (position + i * i) % hm->capacity;
  }

  hm->items[position].hash = hash;
  hm->items[position].key = key;
  hm->items[position].value = value;

  return &hm->items[position];
}

INLINE
hashmap_entry *replace(hashmap *hm, TKey key, TValue value) {
  hashmap_entry *entry = find(hm, key);
  if (NULL == entry) {
    return NULL;
  }
  entry->value = value;
  return entry;
}

INLINE
hashmap_entry *put(hashmap *hm, TKey key, TValue value) {

  if (hm->count + 1 > hm->capacity) {
    if (!grow(hm)) {
      return NULL;
    }
  }

  size_t hash = hash_key(key);
  hash = hash == 0 ? 1 : hash;

  size_t position = hash % hm->capacity;
  for (size_t i = 1; i <= hm->capacity; ++i) {

    if (hm->items[position].hash == 0) {
      ++hm->count;
      break; // Not occupied
    }

    if (equals_key(key, hm->items[position].key)) {
      break; // Replace current value.
    }

    position = (position + i * i) % hm->capacity;
  }
  hm->items[position].hash = hash;
  hm->items[position].key = key;
  hm->items[position].value = value;

  return &hm->items[position];
}

INLINE
void remove(hashmap *hm, hashmap_entry *entry) {
  assert(entry - hm->items > 0 && (size_t)(entry - hm->items) < hm->capacity);
  entry->hash = 0;
#ifdef FUNCTION_0
  FUNCTION_0(entry);
#endif
  --hm->count;
}

INLINE
bool delete_(hashmap *hm, TKey key) {
  hashmap_entry *entry = find(hm, key);
  if (NULL == entry) {
    return false;
  }
  remove(hm, entry);
  return true;
}

#endif

#undef TKey
#undef TValue
#undef hash_key
#undef equals_key
#undef hashmap
#undef hashmap_entry
#undef insert
#undef replace
#undef put
#undef remove
#undef delete_
#undef find
#undef destroy
#undef grow
#undef destroy
#include "template-undef.h"
