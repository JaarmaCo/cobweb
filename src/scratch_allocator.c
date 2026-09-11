#if !defined(UNITY_BUILD)
#include "allocator.h"
#endif

#include <stdio.h>
#include <stdlib.h>

static _Thread_local allocator_t *scratch_arena = NULL;

allocator_t *scratch_allocator(size_t buffer_size) {

  if (buffer_size == 0) {
    free(scratch_arena);
    scratch_arena = NULL;
    return NULL;
  }

  void *new_arena = realloc(scratch_arena, buffer_size);
  if (NULL == new_arena) {
    perror("realloc");
    exit(1);
  }
  scratch_arena = arena_allocator(buffer_size, new_arena);
  return scratch_arena;
}
