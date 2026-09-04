#include "allocator.h"
#include <assert.h>
#include <stdlib.h>

static void *malloc_allocate(allocator_t *allocator,
                             const allocation_t *allocation) {
  (void)allocator;
  assert(allocation->new_size == 0 ||
         allocation->new_size > allocation->old_size);
  if (allocation->new_size == 0) {
    free(allocation->memory);
    return NULL;
  } else if (allocation->old_size > 0) {
    return realloc(allocation->memory, allocation->new_size);
  } else {
    return malloc(allocation->new_size);
  }
}

allocator_t malloc_allocator(void) {
  return (allocator_t){
      .allocate = malloc_allocate,
  };
}

static void *debug_allocate(allocator_t *allocator,
                            const allocation_t *allocation) {
  debug_allocator_t *debug_allocator = (debug_allocator_t *)allocator;
  debug_allocator->callback(allocation, (void *)debug_allocator->base.data);
  return debug_allocator->upstream->allocate(allocator, allocation);
}

debug_allocator_t debug_allocator(allocator_t *upstream,
                                  allocation_debug_function_t debug,
                                  void *user) {
  return (debug_allocator_t){
      .upstream = upstream,
      .base =
          {
              .allocate = debug_allocate,
              .data = (uintptr_t)user,
          },
      .callback = debug,
  };
}

static void trace_allocation(const allocation_t *allocation, void *user) {
  size_t *out_balance = (size_t *)user;
  if (allocation->new_size >= allocation->old_size) {
    *out_balance += allocation->new_size - allocation->old_size;
  } else {
    *out_balance -= allocation->old_size;
  }
}

debug_allocator_t trace_free_balance_allocator(allocator_t *upstream,
                                               size_t *out_balance) {
  *out_balance = 0;
  return debug_allocator(upstream, trace_allocation, out_balance);
}

void *allocator_new(allocator_t *allocator, size_t size, size_t alignment) {
  allocation_t arg = {
      .memory = NULL,
      .old_size = 0,
      .new_size = size,
      .alignment = alignment,
  };
  return allocator->allocate(allocator, &arg);
}

void *allocator_resize(allocator_t *allocator, void *hint, size_t old_size,
                       size_t new_size, size_t alignment) {
  allocation_t arg = {
      .memory = hint,
      .old_size = old_size,
      .new_size = new_size,
      .alignment = alignment,
  };
  return allocator->allocate(allocator, &arg);
}

void allocator_release(allocator_t *allocator, void *memory, size_t size,
                       size_t alignment) {
  allocation_t arg = {
      .memory = memory,
      .old_size = size,
      .new_size = 0,
      .alignment = alignment,
  };
  allocator->allocate(allocator, &arg);
}
