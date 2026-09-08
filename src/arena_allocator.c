#include "allocator.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct arena_header {
  void *arena_pointer;
  void *arena_end;
};

static void *arena_allocate(allocator_t *arena,
                            const allocation_t *allocation) {
  assert(allocation->old_size <= allocation->new_size);
  if (allocation->new_size == 0) {
    return NULL;
  }

  struct arena_header *header = (struct arena_header *)arena->data;

  if (allocation->memory ==
      (char *)header->arena_pointer - allocation->old_size) {
    // Points to the end of the arena, memory may be extended
    //
    size_t remaining =
        (uintptr_t)header->arena_end - (uintptr_t)allocation->memory;
    if (remaining < allocation->new_size) {
      return NULL;
    }
    header->arena_pointer = (char *)allocation->memory + allocation->new_size;
    return allocation->memory;
  }

  uintptr_t start_address = (uintptr_t)header->arena_pointer;
  if (start_address % allocation->alignment != 0) {
    start_address +=
        allocation->alignment - start_address % allocation->alignment;
  }

  uintptr_t end_address = start_address + allocation->new_size;
  if (end_address > (uintptr_t)header->arena_end) {
    return NULL;
  }
  header->arena_pointer = (void *)end_address;

  if (allocation->old_size != 0) {
    // Ensure the old data is copied over if this is a realloc-style allocation.
    //
    memcpy((void *)start_address, allocation->memory, allocation->old_size);
  }

  return (void *)start_address;
}

allocator_t *arena_allocator(size_t buffer_size, void *buffer) {
  assert(buffer_size >= 128);
  assert((uintptr_t)buffer % _Alignof(max_align_t) == 0);

  if (NULL == buffer) {
    buffer = malloc(buffer_size);
    if (NULL == buffer) {
      perror("malloc");
      exit(1);
    }
  }

  allocator_t *allocator = buffer;
  allocator->data = (uintptr_t)(allocator + 1);
  allocator->allocate = arena_allocate;

  struct arena_header *header = (struct arena_header *)allocator->data;
  header->arena_pointer = header + 1;
  header->arena_end =
      (char *)header->arena_pointer +
      (buffer_size - sizeof(allocator_t) - sizeof(struct arena_header));

  return allocator;
}
