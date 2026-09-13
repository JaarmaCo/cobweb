#include "concurrency/sync.h"
#include "concurrency/thread.h"

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

static mutex_t *mutex = NULL;

static void run(void *arg) {
  int *count = arg;
  char tname_buf[THREAD_MAX_NAME_LENGTH + 1];

  printf(INFO("%s: mutex_lock"),
         thread_get_name(thread_get_this(), tname_buf, NULL));
  mutex_lock(mutex);
  {
    ++*count;
  }
  printf(INFO("%s: mutex_unlock"),
         thread_get_name(thread_get_this(), tname_buf, NULL));
  mutex_unlock(mutex);
}

int main(void) {
  int count = 0;
  size_t allocated = 0;
  debug_allocator_t dballoc =
      trace_free_balance_allocator(xmalloc_allocator, &allocated);

  thread_t *thread = thread_create(&dballoc.base, run, &count);
  if (NULL == thread) {
    fprintf(stderr, ERR("Failed to create the thread"));
    exit(1);
  }
  thread_set_name(thread, SV("Worker"));

  mutex = mutex_create(&dballoc.base);

  printf(INFO("Main: mutex_lock"));
  mutex_lock(mutex);
  {
    thread_start(thread);

    if (count != 0) {
      fprintf(stderr,
              ERR("Race detected, count was incremented prematurely: %d != 0"),
              count);
      exit(1);
    }
  }
  printf(INFO("Main: mutex_unlock"));
  mutex_unlock(mutex);

  thread_await(thread);

  printf(INFO("Main: mutex_try_lock"));
  if (!mutex_try_lock(mutex)) {
    fprintf(stderr, ERR("Thread would have released the lock, as it was waited "
                        "on, but mutex_try_lock failed to acquire it."));
    exit(1);
  }
  {
    if (count != 1) {
      fprintf(stderr,
              ERR("Thread should have gained access to the mutex, and "
                  "incremented it, however: %d != 1"),
              count);
      exit(1);
    }
  }
  printf(INFO("Main: mutex_unlock"));
  mutex_unlock(mutex);

  thread_join(thread);
  mutex_destroy(mutex);

  if (allocated != 0) {
    fprintf(stderr, ERR("Leaked %zu bytes"), allocated);
    exit(1);
  }
  printf(OK("Mutex sequence validated"));
  return 0;
}
