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
#define append_range da_append_range
#define insert da_insert
#define insert_range da_insert_range
#define remove_back_range da_remove_back_range
#define remove_back da_remove_back
#define remove da_remove
#define remove_range da_remove_range
#define destroy da_destroy

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
 * Destroy the provided dynamic array.
 */
void destroy(dynamic_array *array);

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
 * Append a range of elements to the end of the array.
 *
 * @return A pointer to the first added element, or NULL if a memory allocation
 *         failed.
 */
T *append_range(dynamic_array *array, size_t count, const T *items);

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
 * Remove the last element from the array.
 *
 * @return The removed element.
 */
T remove_back(dynamic_array *array);

/**
 * Remove the last count elements from the array.
 */
void remove_back_range(dynamic_array *array, size_t count);

/**
 * Remove an element at a specified index.
 *
 * @param index Index of the element to remove.
 * @return The removed element.
 */
T remove(dynamic_array *array, size_t index);

/**
 * Erase a range of elements in an array by first shifting elements
 * to the left, and then shrinking the array.
 *
 * @param index Index of the first element to remove.
 * @param count Number of elements to remove.
 */
void remove_range(dynamic_array *array, size_t index, size_t count);

//! Template C "dynamic_array_${T}.c"
//! Template INCLUDE "dynamic_array_${T}.h"

inline void destroy(dynamic_array *array) {
  if (NULL == array || NULL == array->items) {
    return;
  }
  allocator_release(array->allocator, array->items, array->capacity * sizeof(T),
                    _Alignof(T));
  memset(array, 0, sizeof *array);
}

inline size_t reserve(dynamic_array *array, size_t size) {
  assert(NULL != array);
  assert(array->count <= array->capacity);
  assert(array->capacity == 0 || NULL != array->items);

  if (size <= array->capacity) {
    return array->capacity;
  }

  size = size + size / 2;
  T *new_memory = (T *)allocator_resize(array->allocator, array->items,
                                        array->capacity * sizeof(T),
                                        size * sizeof(T), _Alignof(T));
  if (NULL == new_memory) {
    return 0;
  }
  array->items = new_memory;
  array->capacity = size;
  return size;
}

inline T *append(dynamic_array *array, T item) {
  assert(NULL != array);
  assert(array->count <= array->capacity);
  assert(array->capacity == 0 || NULL != array->items);

  if (!reserve(array, array->count + 1)) {
    return NULL;
  }
  T *end = &array->items[array->count++];
  *end = item;
  return end;
}

inline T *append_range(dynamic_array *array, size_t count, const T *items) {
  assert(NULL != array);
  assert(array->count <= array->capacity);
  assert(array->capacity == 0 || NULL != array->items);
  assert(count == 0 || NULL != items);

  if (count == 0) {
    return NULL;
  }

  if (!reserve(array, array->count + count)) {
    return NULL;
  }

  T *first = array->items + array->count;
  memcpy(first, items, count * sizeof(T));

  array->count += count;
  return first;
}

inline T remove_back(dynamic_array *array) {
  T back = array->items[array->count - 1];
  remove_back_range(array, 1);
  return back;
}

inline void remove_back_range(dynamic_array *array, size_t count) {
  assert(NULL != array);
  assert(array->count <= array->capacity);
  assert(array->capacity == 0 || NULL != array->items);
  assert(array->count >= count);

  array->count -= count;
}

inline T remove(dynamic_array *array, size_t index) {
  assert(index < array->count);

  T item = array->items[index];
  remove_range(array, index, 1);
  return item;
}

inline void remove_range(dynamic_array *array, size_t index, size_t count) {
  assert(NULL != array);
  assert(array->count <= array->capacity);
  assert(array->capacity == 0 || NULL != array->items);
  assert(index < array->count);
  assert(index + count <= array->count);

  memmove(array->items + index, array->items + index + count,
          (array->count - index - count) * sizeof(T));
  remove_back_range(array, count);
}

inline T *insert_range(dynamic_array *array, size_t index, size_t count,
                       const T *items) {
  assert(NULL != array);
  assert(array->count <= array->capacity);
  assert(array->capacity == 0 || NULL != array->items);
  assert(index < array->count);
  assert(count == 0 || NULL != items);

  if (!reserve(array, array->count + count)) {
    return NULL;
  }
  memmove(array->items + index + count, array->items + index,
          (array->count - index) * sizeof(T));
  memcpy(array->items + index, items, count * sizeof(T));
  array->count += count;
  return array->items + index;
}

inline T *insert(dynamic_array *array, size_t index, T item) {
  return insert_range(array, index, 1, &item);
}
