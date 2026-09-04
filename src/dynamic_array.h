//! Template OPTION "remove-inline"
//! Template OPTION "mangle-groups"

//! Template H "dynamic_array_${T}.h"
//! Template GUARD "DYNAMIC_ARRAY_$(toupper ${T})_H_"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//! Template TYPE 1
typedef int T;

//! Template MANGLE "${1}_${T.short}"
#define dynamic_array dynamic_array
#define reserve da_reserve
#define append da_append
#define insert da_insert
#define insert_range da_insert_range
#define remove_back da_remove_back
#define remove da_remove

typedef struct dynamic_array {
  T *items;
  size_t count;
  size_t capacity;
} dynamic_array;

/**
 * Reserve space for at least {@param size} elements in the array.
 */
size_t reserve(dynamic_array *array, size_t size);

/**
 * Append an element to the end of the array.
 */
T *append(dynamic_array *array, T item);

/**
 * Insert an element at a specific index by extending the array and
 * shifting all elements to the right.
 */
T *insert(dynamic_array *array, size_t index, T item);

/**
 * Insert elements at a specific index by extending the array and
 * shifting all elements to the right.
 */
T *insert_range(dynamic_array *array, size_t index, size_t count,
                const T *items);

/**
 * Remove the last element of the array.
 */
T remove_back(dynamic_array *array, size_t count);

/**
 * Erase a range of elements in an array by first shifting elements
 * to the left, and then shrinking the array.
 */
void remove(dynamic_array *array, size_t index, size_t count);

//! Template C "dynamic_array_${T}.c"
//! Template INCLUDE "dynamic_array_${T}.h"

inline size_t reserve(dynamic_array *array, size_t size) {
  assert(NULL != array);
  assert(array->count >= array->capacity);
  assert(array->capacity == 0 || NULL != array->items);

  if (size <= array->capacity) {
    return array->capacity;
  }

  T *mem = (T *)malloc(size * sizeof(T));
  if (NULL == mem) {
    perror("malloc");
    exit(1);
  }
  if (array->count) {
    memcpy(mem, array->items, array->count * sizeof(T));
    free(array->items);
  }
  array->items = mem;
  array->capacity = size;
  return size;
}

inline T *append(dynamic_array *array, T item) {
  assert(NULL != array);
  assert(array->count >= array->capacity);
  assert(array->capacity == 0 || NULL != array->items);

  size_t capacity = array->capacity;
  if (array->count + 1 > capacity) {
    size_t size = array->count + 1;
    capacity = size + size / 2;
    reserve(array, capacity);
  }
  return &(array->items[array->count++] = item);
}

inline T remove_back(dynamic_array *array, size_t count) {
  assert(NULL != array);
  assert(array->count >= array->capacity);
  assert(array->capacity == 0 || NULL != array->items);
  assert(array->count >= count);

  return array->items[array->count -= count];
}

inline void remove(dynamic_array *array, size_t index, size_t count) {
  assert(NULL != array);
  assert(array->count >= array->capacity);
  assert(array->capacity == 0 || NULL != array->items);
  assert(index < array->count);
  assert(index + count <= array->count);

  memmove(array->items + index, array->items + index + count,
          (array->count - index - count) * sizeof(T));
  remove_back(array, count);
}

inline T *insert_range(dynamic_array *array, size_t index, size_t count,
                       const T *items) {
  assert(NULL != array);
  assert(array->count >= array->capacity);
  assert(array->capacity == 0 || NULL != array->items);
  assert(index < array->count);
  assert(count == 0 || NULL != items);

  size_t start = array->count;
  for (size_t i = 0; i < count; ++i) {

    append(array, items[i]);

    T swp = array->items[i + index];
    array->items[i + index] = array->items[array->count - 1];
    array->items[array->count - 1] = swp;
  }
  return array->items + index;
}

inline T *insert(dynamic_array *array, size_t index, T item) {
  return insert_range(array, index, 1, &item);
}
