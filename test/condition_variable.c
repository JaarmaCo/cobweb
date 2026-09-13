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
  mutex_t *mutex;
  condition_variable_t *condition;
  int ready;
  int go;
} test_context_t;

void run(void *arg) {
  test_context_t *ctx = arg;

  mutex_lock(ctx->mutex);

  /*
   * Tell main that we are ready to wait.  We deliberately keep the
   * mutex locked and immediately enter condition_variable_wait().
   * condition_variable_wait() must atomically release the mutex while
   * putting this thread to sleep.
   */
  ctx->ready = 1;
  condition_variable_signal(ctx->condition);

  while (!ctx->go) {
    condition_variable_wait(ctx->condition, ctx->mutex);
  }

  ++(*ctx->count);

  mutex_unlock(ctx->mutex);
}

int main(void) {
  int count = 0;
  size_t allocated = 0;

  debug_allocator_t dballoc =
      trace_free_balance_allocator(xmalloc_allocator, &allocated);

  mutex_t *mutex = mutex_create(&dballoc.base);
  condition_variable_t *condition = condition_variable_create(&dballoc.base);

  if (NULL == mutex || NULL == condition) {
    fprintf(stderr, ERR("Failed to create synchronization primitives"));

    if (condition != NULL)
      condition_variable_destroy(condition);
    if (mutex != NULL)
      mutex_destroy(mutex);

    if (allocated != 0)
      fprintf(stderr, ERR("Leaked %zu bytes"), allocated);

    exit(1);
  }

  test_context_t context = {
      .count = &count,
      .mutex = mutex,
      .condition = condition,
      .ready = 0,
      .go = 0,
  };

  thread_t *thread = thread_create(&dballoc.base, run, &context);

  if (NULL == thread) {
    fprintf(stderr, ERR("Failed to create the thread"));

    mutex_destroy(mutex);
    condition_variable_destroy(condition);

    if (allocated != 0)
      fprintf(stderr, ERR("Leaked %zu bytes"), allocated);

    exit(1);
  }

  thread_start(thread);

  /*
   * Wait until the worker has reached its condition-variable wait.
   *
   * If the worker hasn't set ready yet, this wait releases the mutex,
   * allowing the worker to acquire it. Once ready is signalled, the
   * worker proceeds into its own condition_variable_wait(), releasing
   * the mutex and allowing us to continue.
   */
  mutex_lock(mutex);

  while (!context.ready) {
    condition_variable_wait(condition, mutex);
  }

  /*
   * Change the predicate and wake the worker.
   */
  context.go = 1;
  condition_variable_signal(condition);

  mutex_unlock(mutex);

  thread_join(thread);

  if (count != 1) {
    fprintf(stderr, ERR("Expected count to be 1, got %d"), count);
    mutex_destroy(mutex);
    condition_variable_destroy(condition);
    exit(1);
  }

  fprintf(stdout, OK("Condition variable wait/signal test passed"));

  mutex_destroy(mutex);
  condition_variable_destroy(condition);

  if (allocated != 0) {
    fprintf(stderr, ERR("Leaked %zu bytes"), allocated);
    exit(1);
  }

  fprintf(stdout, OK("No memory leaked"));
  return 0;
}
