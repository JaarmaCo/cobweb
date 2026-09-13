
#include "concurrency/sync.h"
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

typedef struct {
  int *count;
  semaphore_t *semaphore;
  semaphore_t *started;
} test_context_t;

void run(void *arg) {
  test_context_t *ctx = arg;

  /*
   * Tell the main thread that we are about to acquire the tested
   * semaphore.
   */
  semaphore_release(ctx->started);

  /*
   * The tested semaphore starts at zero, so this must block until
   * the main thread releases it.
   */
  semaphore_acquire(ctx->semaphore);

  ++(*ctx->count);

  semaphore_release(ctx->semaphore);
}

int main(void) {
  int count = 0;
  size_t allocated = 0;

  debug_allocator_t dballoc =
      trace_free_balance_allocator(xmalloc_allocator, &allocated);

  /*
   * The tested semaphore starts at zero. Therefore semaphore_acquire()
   * must block until semaphore_release() is called.
   */
  semaphore_t *semaphore = semaphore_create(&dballoc.base, 0);

  /*
   * This semaphore is only used to synchronize the test with the
   * worker, so that we don't need a scheduler/yield operation.
   */
  semaphore_t *started = semaphore_create(&dballoc.base, 0);

  if (NULL == semaphore || NULL == started) {
    fprintf(stderr, ERR("Failed to create semaphore"));

    if (started != NULL)
      semaphore_destroy(started);
    if (semaphore != NULL)
      semaphore_destroy(semaphore);

    exit(1);
  }

  test_context_t context = {
      .count = &count,
      .semaphore = semaphore,
      .started = started,
  };

  /*
   * Threads are suspended after creation.
   */
  thread_t *thread = thread_create(&dballoc.base, run, &context);

  if (NULL == thread) {
    fprintf(stderr, ERR("Failed to create the thread"));

    semaphore_destroy(started);
    semaphore_destroy(semaphore);
    exit(1);
  }

  thread_start(thread);

  /*
   * This guarantees that the worker has reached the point immediately
   * before semaphore_acquire(). Since the tested semaphore still has
   * value zero, the worker cannot have incremented count.
   */
  semaphore_acquire(started);

  if (count != 0) {
    fprintf(stderr, ERR("Thread acquired semaphore before it was released"));

    thread_join(thread);
    semaphore_destroy(started);
    semaphore_destroy(semaphore);
    exit(1);
  }

  fprintf(stdout, OK("Thread blocked on semaphore"));

  /*
   * Allow the worker to proceed.
   */
  semaphore_release(semaphore);

  thread_join(thread);

  if (count != 1) {
    fprintf(stderr, ERR("Expected count to be 1, got %d"), count);

    semaphore_destroy(started);
    semaphore_destroy(semaphore);
    exit(1);
  }

  fprintf(stdout, OK("Semaphore acquire/release test passed"));

  semaphore_destroy(started);
  semaphore_destroy(semaphore);

  if (allocated != 0) {
    fprintf(stderr, ERR("Leaked %zu bytes"), allocated);
    exit(1);
  }

  fprintf(stdout, OK("No memory leaked"));
  return 0;
}
