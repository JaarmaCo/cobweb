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

static void init(void *arg) {
  char name_buffer[THREAD_MAX_NAME_LENGTH + 1];
  int *count = arg;
  printf(INFO("init(%d) - %s"), *count,
         thread_get_name(thread_get_this(), name_buffer, NULL));
  ++*count;
}

static void cleanup(void *arg) {
  char name_buffer[THREAD_MAX_NAME_LENGTH + 1];
  int *count = arg;
  printf(INFO("cleanup(%d) - %s"), *count,
         thread_get_name(thread_get_this(), name_buffer, NULL));
  ++*count;
}

static void run(void *arg) {
  char name_buffer[THREAD_MAX_NAME_LENGTH + 1];
  int *count = arg;
  printf(INFO("run(%d) - %s"), *count,
         thread_get_name(thread_get_this(), name_buffer, NULL));
  ++*count;
}

int main(void) {
  int count = 0;
  size_t allocated_size_1 = 0;
  size_t allocated_size_2 = 0;
  debug_allocator_t dballoc1 =
      trace_free_balance_allocator(malloc_allocator, &allocated_size_1);
  debug_allocator_t dballoc2 =
      trace_free_balance_allocator(malloc_allocator, &allocated_size_2);

  thread_t *thread1 = thread_create(&dballoc1.base, run, &count);
  if (NULL == thread1) {
    fprintf(stderr, ERR("Failed to create the thread: %s"), strerror(errno));
    exit(1);
  }
  thread_at_init(thread1, init);
  thread_at_exit(thread1, cleanup);
  thread_set_name(thread1, SV("Thread 1"));

  printf(INFO("thread_create(%d)"), count);

  thread_t *thread2 = thread_create(&dballoc2.base, run, &count);
  if (NULL == thread2) {
    fprintf(stderr, ERR("Failed to create the thread: %s"), strerror(errno));
    exit(1);
  }
  thread_at_init(thread2, init);
  thread_at_exit(thread2, cleanup);
  thread_set_name(thread2, SV("Thread 2"));

  printf(INFO("thread_create(%d)"), count);

  if (count != 0) {
    fprintf(stderr, ERR("A thread started prematurely: %d != 0"), count);
    exit(1);
  }

  thread_start(thread1);
  thread_await(thread1);

  if (count != 2) {
    fprintf(stderr,
            ERR("Attempted to run and wait for thread 1, however thread 1 did "
                "not increment count for both init and run: %d != 2"),
            count);
    exit(1);
  }

  if (thread_get_current_state(thread1) != THREAD_STATE_SUSPENDED) {
    fprintf(stderr, ERR("Thread 1 is supposed to be in the suspended state "
                        "after having been waited on, but isn't."));
    exit(1);
  }

  thread_start(thread1);
  thread_await(thread1);

  if (count != 3) {
    fprintf(stderr,
            ERR("Attempted to run thread 1 again, however thread 1 did not "
                "increment count: %d != 3"),
            count);
    exit(1);
  }

  thread_start(thread2);
  thread_join(thread2);

  if (count != 5) {
    fprintf(stderr,
            ERR("Attempted to run and join with thread 2, however thread 2 did "
                "not increment count for all of init, cleanup: %d != 5"),
            count);
    exit(1);
  }

  if (allocated_size_2 != 0) {
    fprintf(stderr,
            ERR("%zu bytes where not freed after joining with thread 2"),
            allocated_size_2);
    exit(1);
  }

  thread_join(thread1);

  if (count != 6) {
    fprintf(stderr,
            ERR("Attempted to join with thread 1, however thread 1 did not "
                "increment count once: %d != 6"),
            count);
    exit(1);
  }

  if (allocated_size_1 != 0) {
    fprintf(stderr,
            ERR("%zu bytes where not freed after joining with thread 1"),
            allocated_size_1);
    exit(1);
  }

  printf(OK("Thread sequence validated."));
  return 0;
}
