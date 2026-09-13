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

#include "concurrency/sync.h"
#include "concurrency/thread.h"

#define TYPE_2 int
#define PREFIX queue_
#define HEADER_ONLY
#include "concurrency/linked_queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ESC "\x1B"
#define BOLD ESC "[1m"
#define RED ESC "[31m"
#define GREEN ESC "[32m"
#define BLUE ESC "[34m"
#define RST ESC "[0m"

#define ERR(str) BOLD RED "[ERROR]: " RST str "\n"
#define OK(str) BOLD GREEN "[OK]: " RST str "\n"
#define INFO(str) BOLD BLUE str RST "\n"

#define THREAD_COUNT 3
#define VALUE_COUNT 10

static fence_t *start_guard;
static int thread_dequeue_counts[THREAD_COUNT] = {0};
static int thread_dequeue_results[THREAD_COUNT][VALUE_COUNT] = {0};
static concurrent_linked_queue *queue_ptr;

static void producer(void *arg) {
  (void)arg;

  fence_wait(start_guard);

  for (int i = 0; i < VALUE_COUNT; ++i) {
    if (!queue_enqueue(queue_ptr, i)) {
      fprintf(stderr, ERR("Failed to enqueue item"));
      exit(1);
    }
  }
}

static void consumer(void *arg) {
  intptr_t thread_number = (intptr_t)arg;

  fence_wait(start_guard);

  for (int i = 0; i < VALUE_COUNT; ++i) {
    int value;
    for (int j = 0; j < 10; ++j) {
      if (queue_dequeue(queue_ptr, &value)) {
        break;
      }
      usleep(10);
    }
    int valnr = thread_dequeue_counts[thread_number]++;
    thread_dequeue_results[thread_number][valnr] = value;
  }
}

int main(void) {
  size_t allocated = 0;
  debug_allocator_t dballoc =
      trace_free_balance_allocator(xmalloc_allocator, &allocated);

  concurrent_linked_queue queue;
  if (!queue_create(&queue, &dballoc.base)) {
    fprintf(stderr, ERR("Failed to create the queue"));
    exit(1);
  }
  queue_ptr = &queue;

  start_guard = fence_create(&dballoc.base, FENCE_STATE_CLOSED);
  if (NULL == start_guard) {
    fprintf(stderr, ERR("Failed to create fence"));
    exit(1);
  }

  thread_t *producer_threads[THREAD_COUNT];
  thread_t *consumer_threads[THREAD_COUNT];

  for (int i = 0; i < THREAD_COUNT; ++i) {

    producer_threads[i] = thread_create(&dballoc.base, producer, NULL);
    consumer_threads[i] =
        thread_create(&dballoc.base, consumer, (void *)(intptr_t)i);

    if (NULL == producer_threads[i] || NULL == consumer_threads[i]) {
      fprintf(stderr, ERR("Failed to create threads."));
      exit(1);
    }
  }

  for (int i = 0; i < THREAD_COUNT; ++i) {
    thread_start(consumer_threads[i]);
    thread_start(producer_threads[i]);
  }

  fence_signal(start_guard);

  for (int i = 0; i < THREAD_COUNT; ++i) {
    thread_join(producer_threads[i]);
    thread_join(consumer_threads[i]);
  }

  fence_destroy(start_guard);
  queue_destroy(&queue);

  int total_counts = 0;
  int value_counts[VALUE_COUNT] = {0};
  for (int i = 0; i < THREAD_COUNT; ++i) {
    for (int j = 0; j < thread_dequeue_counts[i]; ++j) {

      int value = thread_dequeue_results[i][j];
      ++total_counts;

      if (value < 0 || value >= VALUE_COUNT) {
        fprintf(stderr,
                ERR("Memory corruption detected, value %d is out of range."),
                value);
        exit(1);
      }

      if (++value_counts[value] > THREAD_COUNT) {
        fprintf(
            stderr,
            ERR("Value %d had more occurences than the number of producers. "
                "This indicate two consumers dequeued the same value."),
            value);
        exit(1);
      }
    }
  }

  printf(INFO("Detected %d dequeued values"), total_counts);

  printf(OK("Values have the expected number of occurences"));

  if (allocated != 0) {
    fprintf(stderr, ERR("Leaked %zu bytes"), allocated);
    exit(1);
  }
  printf(OK("No memory leaks detected."));
}
