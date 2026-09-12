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

#include "concurrency/promise.h"
#include "concurrency/thread.h"

#include <stdio.h>
#include <stdlib.h>

#define ESC "\x1B"
#define BOLD ESC "[1m"
#define RED ESC "[31m"
#define GREEN ESC "[32m"
#define BLUE ESC "[34m"
#define RST ESC "[0m"

#define ERR(str) BOLD RED "[ERROR]: " RST str "\n"
#define OK(str) BOLD GREEN "[OK]: " RST str "\n"
#define INFO(str) BOLD BLUE str RST "\n"

static void supplier(void *arg) {
  printf(INFO("promise_complete()"));
  promise_t *promise = arg;
  promise_complete(promise, &(int){42});
}

int main(void) {

  size_t allocated = 0;
  debug_allocator_t dballoc =
      trace_free_balance_allocator(malloc_allocator, &allocated);

  promise_t *promise = promise_create(&dballoc.base, sizeof(int));
  if (NULL == promise) {
    fprintf(stderr, ERR("Failed to create the promise object"));
    exit(1);
  }

  thread_t *thread = thread_create(&dballoc.base, supplier, promise);
  if (NULL == thread) {
    fprintf(stderr, ERR("Failed to create the thread."));
    exit(1);
  }

  future_t *future = promise_get_future(promise);
  if (future_get_status(future) != FUTURE_STATUS_PENDING) {
    fprintf(stderr,
            ERR("Future status should be pending when not yet completed"));
    exit(1);
  }

  printf(INFO("thread_start()"));
  thread_start(thread);

  printf(INFO("future_get()"));
  int value = 0;
  if (!future_get(future, &value)) {
    fprintf(stderr, ERR("Future value should be available, but isn't."));
    exit(1);
  }

  if (value != 42) {
    fprintf(stderr,
            ERR("Value from future_get (%d) did not match the value "
                "provided in promise_complete (42)"),
            value);
  }

  printf(INFO("thread_join()"));
  thread_join(thread);

  if (allocated != 0) {
    fprintf(stderr, ERR("Leaked %zu bytes"), allocated);
    exit(1);
  }

  printf(OK("Promise sequence validated."));
  return 0;
}
