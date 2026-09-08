#include "allocator.h"

#include <string.h>

struct free_header {
  size_t size;
  size_t alignment;
  struct free_header *next;
  struct free_header *prev;
};

struct auto_free_header {
  allocator_t base;
  allocator_t *upstream;
  struct free_header *first;
};

static void *auto_free_new(struct auto_free_header *header, size_t size,
                           size_t alignment) {
  struct free_header *pointer =
      allocator_new(header->upstream, size, alignment);
  if (NULL == pointer) {
    return NULL;
  }

  pointer->next = header->first;
  pointer->prev = NULL;

  pointer->size = size;
  pointer->alignment = alignment;

  header->first->prev = pointer;
  header->first = pointer;
  return pointer + 1;
}

static void auto_free_release(struct auto_free_header *header, void *memory) {

  struct free_header *pointer = (struct free_header *)memory - 1;

  if (pointer->prev) {
    pointer->prev->next = pointer->next;
  }

  if (pointer->next) {
    pointer->next->prev = pointer->prev;
  }

  if (pointer == header->first) {
    header->first = pointer->next;
  }

  allocator_release(header->upstream, pointer, pointer->size,
                    pointer->alignment);
}

static void *auto_free_allocate(allocator_t *allocator,
                                const allocation_t *allocation) {
  size_t size = sizeof(struct free_header) + allocation->new_size;
  size_t alignment = _Alignof(struct free_header) > allocation->alignment
                         ? _Alignof(struct free_header)
                         : allocation->alignment;
  struct auto_free_header *header = (struct auto_free_header *)allocator;

  if (allocation->new_size == 0) {
    auto_free_release(header, allocation->memory);
    return NULL;
  }

  if (allocation->old_size != 0) {
    void *memory = auto_free_new(header, size, alignment);
    if (NULL == memory) {
      return NULL;
    }
    memcpy(memory, allocation->memory, allocation->old_size);
    auto_free_release(header, allocation->memory);
    return memory;
  }
  return auto_free_new(header, size, alignment);
}

allocator_t *auto_free_allocator(allocator_t *upstream) {
  struct auto_free_header *header =
      allocator_new(upstream, sizeof(struct auto_free_header),
                    _Alignof(struct auto_free_header));
  if (NULL == header) {
    return NULL;
  }
  header->first = NULL;
  header->upstream = upstream;
  header->base.data = 0;
  header->base.allocate = auto_free_allocate;
  return &header->base;
}

void auto_free(allocator_t *allocator) {

  struct auto_free_header *header = (struct auto_free_header *)allocator;
  struct free_header *pointer = header->first, *next = NULL;
  while (pointer) {
    next = pointer->next;
    allocator_release(header->upstream, pointer, pointer->size,
                      pointer->alignment);
    pointer = next;
  }
  header->first = NULL;
}

void auto_free_destroy(allocator_t *allocator) {
  auto_free(allocator);
  struct auto_free_header *header = (struct auto_free_header *)allocator;
  allocator_release(header->upstream, header, sizeof(struct auto_free_header),
                    _Alignof(struct auto_free_header));
}
