// ================================================================================
// Copyright © 2026 William Jaarma
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ================================================================================

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

allocator_t *scratch_allocator_pop(void) {
  allocator_t *arena = scratch_arena;
  scratch_arena = NULL;
  return arena;
}

void scratch_allocator_restore(allocator_t *old_allocator) {
  free(scratch_arena);
  scratch_arena = old_allocator;
}
