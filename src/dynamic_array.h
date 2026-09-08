// clang-format off
//
// dynamic_array.h
//
// This is a templated dynamic array implementation that is parameterized using
// user defined macros.
//
// The parameters for this file are:
//
// - #define TYPE_0 // Structure type of the array (optional)
// - #define TYPE_1 // Type of the items in the array (required)
// - #define FUNCTION_0(TYPE_1 *) // Item type destructor function (optional)
// - #define PREFIX // Prefix to prepend to all generated functions
// - #define SUFFIX // Suffix to append to all generated symbols
// - #define C_HEADER // If defined, treats this file as an (unguarded) C header file
// - #define C_SOURCE // If defined, treats this file as a C source file
// - #define HEADER_ONLY // If defined, inlines all functions and includes both the source and header
//
// clang-format on

#if !defined(C_SOURCE) && !defined(C_HEADER) && !defined(HEADER_ONLY)
#define TYPE_1 int
#define SUFFIX _i
#define HEADER_ONLY
#endif

#define REQUIRE_TYPE_1
#include "template-def.h"

#if defined(C_HEADER)

#include "allocator.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(TYPE_0)
#define TYPE_0 M_CAT(dynamic_array, SUFFIX)
#endif

#define T TYPE_1

#define dynamic_array TYPE_0
#define reserve M_NAME(reserve)
#define append M_NAME(append)
#define append_range M_NAME(append_range)
#define insert M_NAME(insert)
#define insert_range M_NAME(insert_range)
#define remove_back_range M_NAME(remove_back_range)
#define remove_back M_NAME(remove_back)
#define pop_back M_NAME(pop_back)
#define remove M_NAME(remove)
#define pop M_NAME(pop)
#define remove_range M_NAME(remove_range)
#define destroy M_NAME(destroy)

#if defined(DEFINE_STRUCT)

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

#endif

/**
 * Destroy the provided dynamic array.
 */
INLINE
void destroy(dynamic_array *array);

/**
 * Reserve space for at least {@param size} elements in the array.
 *
 * @return The new capacity of the array, or 0 if the memory allocation failed.
 */
INLINE
size_t reserve(dynamic_array *array, size_t size);

/**
 * Append an element to the end of the array.
 *
 * @return A pointer to the added element, or NULL if a memory allocation
 *         failed.
 */
INLINE
T *append(dynamic_array *array, T item);

/**
 * Append a range of elements to the end of the array.
 *
 * @return A pointer to the first added element, or NULL if a memory allocation
 *         failed.
 */
INLINE
T *append_range(dynamic_array *array, size_t count, T const *items);

/**
 * Insert an element at a specific index by extending the array and
 * shifting all elements to the right.
 *
 * @return A pointer to the inserted element, or NULL if the memory allocation
 *         failed.
 */
INLINE
T *insert(dynamic_array *array, size_t index, T item);

/**
 * Insert elements at a specific index by extending the array and
 * shifting all elements to the right.
 *
 * @return A pointer to the first inserted element, or NULL if the memory
 *         allocation failed.
 */
INLINE
T *insert_range(dynamic_array *array, size_t index, size_t count,
                T const *items);

/**
 * Remove the last element from the array.
 *
 * @return The removed element.
 * @{
 */
INLINE
T pop_back(dynamic_array *array);

INLINE
void remove_back(dynamic_array *array);
/**
 * @}
 */

/**
 * Remove the last count elements from the array.
 */
INLINE
void remove_back_range(dynamic_array *array, size_t count);

/**
 * Remove an element at a specified index.
 *
 * @param index Index of the element to remove.
 * @return The removed element.
 * @{
 */
INLINE
T pop(dynamic_array *array, size_t index);

INLINE
void remove(dynamic_array *array, size_t index);
/**
 * @}
 */

/**
 * Erase a range of elements in an array by first shifting elements
 * to the left, and then shrinking the array.
 *
 * @param index Index of the first element to remove.
 * @param count Number of elements to remove.
 */
INLINE
void remove_range(dynamic_array *array, size_t index, size_t count);

#endif // defined(C_HEADER)

#if defined(C_SOURCE)

INLINE
void destroy(dynamic_array *array) {
  if (NULL == array || NULL == array->items) {
    return;
  }

#if defined(FUNCTION_0)
  for (size_t i = 0; i < array->count; ++i) {
    FUNCTION_0(&array->items[i]);
  }
#endif

  allocator_release(array->allocator, array->items, array->capacity * sizeof(T),
                    _Alignof(T));
  memset(array, 0, sizeof *array);
}

INLINE
size_t reserve(dynamic_array *array, size_t size) {
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

INLINE
T *append(dynamic_array *array, T item) {
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

INLINE
T *append_range(dynamic_array *array, size_t count, T const *items) {
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

INLINE
T pop_back(dynamic_array *array) {
  assert(array->count > 0);
  return array->items[--array->count];
}

INLINE
void remove_back(dynamic_array *array) { remove_back_range(array, 1); }

INLINE
void remove_back_range(dynamic_array *array, size_t count) {
  assert(NULL != array);
  assert(array->count <= array->capacity);
  assert(array->capacity == 0 || NULL != array->items);
  assert(array->count >= count);

#if defined(FUNCTION_0)
  for (size_t i = array->count - count; i < array->count; ++i) {
    FUNCTION_0(&array->items[i]);
  }
#endif

  array->count -= count;
}

INLINE
T pop(dynamic_array *array, size_t index) {
  assert(index < array->count);

  T item = array->items[index];
  memmove(&array->items[index], &array->items[index + 1],
          (array->count - index - 1) * sizeof(T));
  return item;
}

INLINE
void remove(dynamic_array *array, size_t index) {
  remove_range(array, index, 1);
}

INLINE
void remove_range(dynamic_array *array, size_t index, size_t count) {
  assert(NULL != array);
  assert(array->count <= array->capacity);
  assert(array->capacity == 0 || NULL != array->items);
  assert(index < array->count);
  assert(index + count <= array->count);

#if defined(FUNCTION_0)
  for (size_t i = index; i < count; ++i) {
    FUNCTION_0(&array->items[i]);
  }
#endif

  memmove(array->items + index, array->items + index + count,
          (array->count - index - count) * sizeof(T));
  remove_back_range(array, count);
}

INLINE
T *insert_range(dynamic_array *array, size_t index, size_t count,
                T const *items) {
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

INLINE
T *insert(dynamic_array *array, size_t index, T item) {
  return insert_range(array, index, 1, &item);
}

#endif // defined (C_SOURCE)

#undef T
#undef dynamic_array
#undef reserve
#undef append
#undef append_range
#undef insert
#undef insert_range
#undef remove_back_range
#undef remove_back
#undef pop
#undef pop_back
#undef remove
#undef remove_range
#undef destroy
#include "template-undef.h"
