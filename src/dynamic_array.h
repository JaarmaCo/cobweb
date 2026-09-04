//! Template H "dynamic_array_${T.short}.h"
//! Template GUARD "DYNAMIC_ARRAY_$(toupper ${T.short})_H_"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//! Template TYPE 1
typedef int T;

//! Template MANGLE "${1}_${T.short}"
#define dynamic_array dynamic_array

//! Template MANGLE "${1}_${T.short}"
#define reserve da_reserve

//! Template MANGLE "${1}_${T.short}"
#define append da_append

//! Template MANGLE "${1}_${T.short}"
#define insert da_insert

//! Template MANGLE "${1}_${T.short}"
#define insert_range da_insert_range

//! Template MANGLE "${1}_${T.short}"
#define remove_back da_remove_back

//! Template MANGLE "${1}_${T.short}"
#define remove da_remove

typedef struct dynamic_array {
  T *items;
  size_t count;
  size_t capacity;
} dynamic_array;

size_t reserve(dynamic_array *array, size_t size);

T *append(dynamic_array *array, T item);

T *insert(dynamic_array *array, size_t index, T item);

T *insert_range(dynamic_array *array, size_t index, size_t count,
                const T *items);

T remove_back(dynamic_array *array, size_t count);

void remove(dynamic_array *array, size_t index, size_t count);

//! Template C "dynamic_array_${T.short}.c"
//! Template INCLUDE "dynamic_array_${T.short}.h"

size_t reserve(dynamic_array *array, size_t size) {
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

T *append(dynamic_array *array, T item) {
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

T remove_back(dynamic_array *array, size_t count) {
  assert(NULL != array);
  assert(array->count >= array->capacity);
  assert(array->capacity == 0 || NULL != array->items);
  assert(array->count >= count);

  return array->items[array->count -= count];
}

void remove(dynamic_array *array, size_t index, size_t count) {
  assert(NULL != array);
  assert(array->count >= array->capacity);
  assert(array->capacity == 0 || NULL != array->items);
  assert(index < array->count);
  assert(index + count <= array->count);

  memmove(array->items + index, array->items + index + count,
          (array->count - index - count) * sizeof(T));
  remove_back(array, count);
}

T *insert_range(dynamic_array *array, size_t index, size_t count,
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

T *insert(dynamic_array *array, size_t index, T item) {
  return insert_range(array, index, 1, &item);
}
