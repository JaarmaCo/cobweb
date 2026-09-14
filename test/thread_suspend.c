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

#include "concurrency/thread.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ESC "\x1B"
#define BOLD ESC "[1m"
#define RED ESC "[31m"
#define GREEN ESC "[32m"
#define BLUE ESC "[34m"
#define RST ESC "[0m"

#define ERR(str) BOLD RED "[ERROR]: " RST str "\n"
#define OK(str) BOLD GREEN "[OK]: " RST str "\n"
#define INFO(str) BOLD BLUE str RST "\n"

static void run(void *arg) {
  int *count = arg;

  ++*count;
  printf(INFO("Thread: enter"));
  thread_suspend();

  ++*count;
  printf(INFO("Thread: resume 1"));
  thread_suspend();

  ++*count;
  printf(INFO("Thread: resume 2"));
}

int main(void) {
  int count = 0;
  size_t allocated = 0;

  debug_allocator_t dballoc =
      trace_free_balance_allocator(xmalloc_allocator, &allocated);

  thread_t *thread = thread_create(&dballoc.base, run, &count);
  if (NULL == thread) {
    printf(ERR("Failed to create the thread."));
    exit(1);
  }

  printf(INFO("Main: signal 1"));
  thread_start(thread);
  thread_await(thread);

  if (count != 1) {
    printf(ERR("Expected count to be 1, was %d"), count);
    exit(1);
  }

  printf(INFO("Main: signal 2"));
  thread_start(thread);
  thread_await(thread);

  if (count != 2) {
    printf(ERR("Expected count to be 2, was %d"), count);
    exit(1);
  }

  printf(INFO("Main: signal 3"));
  thread_start(thread);
  thread_join(thread);

  if (count != 3) {
    printf(ERR("Expected count to be 3, was %d"), count);
    exit(1);
  }

  printf(OK("Suspension validated"));

  if (allocated != 0) {
    printf(ERR("Leaked %zu bytes"), allocated);
    exit(1);
  }
  printf(OK("No memory leaks detected"));
}
