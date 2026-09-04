//! Template OPTION "remove-inline"
//! Template OPTION "mangle-groups"

//! Template H "dynamic_array_${T}.h"
//! Template GUARD "DYNAMIC_ARRAY_$(toupper ${T})_H_"

#include "allocator.h"

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

/**
 * Templated dynamic array implementation that uses a custom allocator for
 * memory allocations.
 *
 * By default, this implementation will not handle any allocation failures,
 * and will instead return error values should an allocation fail.
 *
 * The memory that was allocated in a dynamic array should either be freed
 * by calling allocator_release on the items field, or freed externally with
 * allocator destruction.
 */
typedef struct dynamic_array {
  /**
   * Pointer to the dynamically allocated array.
   */
  T *items;

  /**
   * Number of items in the array.
   */
  size_t count;

  /**
   * Actual size of the allocated array.
   */
  size_t capacity;

  /**
   * Pointer to a memory allocator that is used when growing the array.
   *
   * If set to NULL, the implementation will instead use realloc for memory
   * allocations.
   */
  allocator_t *allocator;

} dynamic_array;

/**
 * Reserve space for at least {@param size} elements in the array.
 *
 * @return The new capacity of the array, or 0 if the memory allocation failed.
 */
size_t reserve(dynamic_array *array, size_t size);

/**
 * Append an element to the end of the array.
 *
 * @return A pointer to the added element, or NULL if a memory allocation
 *         failed.
 */
T *append(dynamic_array *array, T item);

/**
 * Insert an element at a specific index by extending the array and
 * shifting all elements to the right.
 *
 * @return A pointer to the inserted element, or NULL if the memory allocation
 *         failed.
 */
T *insert(dynamic_array *array, size_t index, T item);

/**
 * Insert elements at a specific index by extending the array and
 * shifting all elements to the right.
 *
 * @return A pointer to the first inserted element, or NULL if the memory
 *         allocation failed.
 */
T *insert_range(dynamic_array *array, size_t index, size_t count,
                const T *items);

/**
 * Remove the last element of the array.
 *
 * @return The removed element
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

  T *new_memory = (T *)allocator_resize(array->allocator, array->items,
                                        array->capacity * sizeof(T),
                                        size * sizeof(T), alignof(T));
  if (NULL == new_memory) {
    return 0;
  }
  array->items = new_memory;
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
    if (!reserve(array, capacity)) {
      return NULL;
    }
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

    if (!append(array, items[i])) {
      return NULL;
    }

    T swp = array->items[i + index];
    array->items[i + index] = array->items[array->count - 1];
    array->items[array->count - 1] = swp;
  }
  return array->items + index;
}

inline T *insert(dynamic_array *array, size_t index, T item) {
  return insert_range(array, index, 1, &item);
}
