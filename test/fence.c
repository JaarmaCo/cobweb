
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
  fence_t *fence;
  semaphore_t *started;
  int *count;
} test_context_t;

void run(void *arg) {
  test_context_t *ctx = arg;

  /*
   * Let the main thread know that the worker is about to wait.
   */
  semaphore_release(ctx->started);

  /*
   * The fence starts unsignaled, so this must block until the main
   * thread calls fence_signal().
   */
  fence_wait(ctx->fence);

  ++(*ctx->count);
}

int main(void) {
  int count = 0;
  size_t allocated = 0;

  debug_allocator_t dballoc =
      trace_free_balance_allocator(xmalloc_allocator, &allocated);

  fence_t *fence = fence_create(&dballoc.base, FENCE_STATE_CLOSED);
  semaphore_t *started = semaphore_create(&dballoc.base, 0);

  if (NULL == fence || NULL == started) {
    fprintf(stderr, ERR("Failed to create synchronization primitives"));

    if (started != NULL)
      semaphore_destroy(started);
    if (fence != NULL)
      fence_destroy(fence);

    exit(1);
  }

  /*
   * Verify that a newly-created fence is unsignaled.
   *
   * fence_try_wait() should fail without blocking.
   */
  if (fence_try_wait(fence)) {
    fprintf(stderr, ERR("Newly-created fence is unexpectedly signaled"));

    semaphore_destroy(started);
    fence_destroy(fence);
    exit(1);
  }

  fprintf(stdout, OK("Newly-created fence is unsignaled"));

  test_context_t context = {
      .fence = fence,
      .started = started,
      .count = &count,
  };

  /*
   * Threads are suspended after creation.
   */
  thread_t *thread = thread_create(&dballoc.base, run, &context);

  if (NULL == thread) {
    fprintf(stderr, ERR("Failed to create the thread"));

    semaphore_destroy(started);
    fence_destroy(fence);
    exit(1);
  }

  thread_start(thread);

  /*
   * Wait until the worker has reached fence_wait().
   */
  semaphore_acquire(started);

  /*
   * The fence is still unsignaled, so the worker must not have
   * passed fence_wait().
   */
  if (count != 0) {
    fprintf(stderr, ERR("Thread passed fence_wait() before fence_signal()"));

    thread_join(thread);
    semaphore_destroy(started);
    fence_destroy(fence);
    exit(1);
  }

  fprintf(stdout, OK("Thread blocked on unsignaled fence"));

  /*
   * Signal the fence. This should allow the worker to pass
   * fence_wait().
   */
  fence_signal(fence);

  thread_join(thread);

  if (count != 1) {
    fprintf(stderr, ERR("Expected count to be 1 after fence_signal(), got %d"),
            count);

    semaphore_destroy(started);
    fence_destroy(fence);
    exit(1);
  }

  fprintf(stdout, OK("fence_signal() released waiting thread"));

  /*
   * Once signaled, fence_try_wait() should succeed without blocking.
   */
  if (!fence_try_wait(fence)) {
    fprintf(stderr, ERR("Signaled fence rejected fence_try_wait()"));

    semaphore_destroy(started);
    fence_destroy(fence);
    exit(1);
  }

  fprintf(stdout, OK("Signaled fence passes fence_try_wait()"));

  /*
   * Reset the fence and verify that it returns to the unsignaled
   * state.
   */
  fence_reset(fence);

  if (fence_try_wait(fence)) {
    fprintf(stderr, ERR("fence_reset() did not reset the fence"));

    semaphore_destroy(started);
    fence_destroy(fence);
    exit(1);
  }

  fprintf(stdout, OK("fence_reset() returned fence to unsignaled state"));

  /*
   * Signal it once more and verify that the fence can be reused.
   */
  fence_signal(fence);

  if (!fence_try_wait(fence)) {
    fprintf(stderr, ERR("Fence could not be signaled after reset"));

    semaphore_destroy(started);
    fence_destroy(fence);
    exit(1);
  }

  fprintf(stdout, OK("Fence can be reused after reset"));

  semaphore_destroy(started);
  fence_destroy(fence);

  if (allocated != 0) {
    fprintf(stderr, ERR("Leaked %zu bytes"), allocated);
    exit(1);
  }

  fprintf(stdout, OK("No memory leaked"));
  return 0;
}
