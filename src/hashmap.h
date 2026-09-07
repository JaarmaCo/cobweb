//! Template OPTION "mangle-groups"
//! Template OPTION "remove-inline"

//! Template H "hashmap_${TKey.short}_${TValue.short}.h"
//! Template GUARD "HASHMAP_$(toupper ${TKey})_$(toupper ${TValue})_H_"

#include <stdbool.h>
#include <stdint.h>

#include "allocator.h"
#include "hash.h"

//! Template TYPE 1
typedef int TKey;

//! Template TYPE 2
typedef int TValue;

//! Template MANGLE "${TKey.hash}"
#define hash_key hash_int

//! Template MANGLE "${TKey.equals}"
#define equals_key equals_int

//! Template MANGLE "${1}_${TKey.short}_${TValue.short}"
#define hashmap hashmap
#define hashmap_entry hashmap_entry
#define insert hm_insert
#define replace hm_replace
#define put hm_put
#define remove hm_remove
#define delete_ hm_delete
#define find hm_find
#define destroy hm_destroy
#define grow hm_grow
#define destroy hm_destroy

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

/**
 * Free all memory owned by the given hashmap.
 */
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
hashmap_entry *put(hashmap *hm, TKey key, TValue value);

/**
 * Finds an existing entry in the hashmap.
 *
 * @param hm Hashmap to seek through.
 * @param key Key to look for.
 *
 * @return A pointer to the located entry, or NULL if no entry was found.
 */
hashmap_entry *find(const hashmap *hm, TKey key);

/**
 * Remove an existing entry from the hashmap.
 *
 * @param hm Hashmap to remove an entry from.
 * @param entry Pointer to the entry to remove.
 */
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
bool delete_(hashmap *hm, TKey key);

//! Template C "hashmap_${TKey.short}_${TValue.short}.c"
//! Template INCLUDE "hashmap_${TKey.short}_${TValue.short}.h"

#include <assert.h>
#include <string.h>

void destroy(hashmap *hm) {
  if (!hm || !hm->items) {
    return;
  }
  allocator_release(hm->allocator, hm->items,
                    hm->capacity * sizeof(hashmap_entry),
                    _Alignof(hashmap_entry));
  memset(hm, 0, sizeof *hm);
}

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

hashmap_entry *replace(hashmap *hm, TKey key, TValue value) {
  hashmap_entry *entry = find(hm, key);
  if (NULL == entry) {
    return NULL;
  }
  entry->value = value;
  return entry;
}

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

void remove(hashmap *hm, hashmap_entry *entry) {
  assert(entry - hm->items > 0 && (size_t)(entry - hm->items) < hm->capacity);
  entry->hash = 0;
  --hm->count;
}

bool delete_(hashmap *hm, TKey key) {
  hashmap_entry *entry = find(hm, key);
  if (NULL == entry) {
    return false;
  }
  remove(hm, entry);
  return true;
}
