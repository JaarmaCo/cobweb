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
#include "thread.h"
#endif

#include <assert.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdatomic.h>
#include <string.h>

struct thread {

  allocator_t *allocator;
  void (*run)(void *);
  void (*at_exit)(void *);
  void (*at_init)(void *);
  void *arg;
  pthread_t impl;

  struct {
    char data[256];
    size_t count;
  } name;

  atomic_size_t iteration;
  jmp_buf unwind_buffer;

  volatile thread_state_t state;
  pthread_mutex_t lock;
  pthread_cond_t resume_condition;
  pthread_cond_t suspend_condition;
};

static _Thread_local thread_t *this_thread_ = NULL;

thread_t *thread_get_this(void) { return this_thread_; }

allocator_t *thread_get_allocator(thread_t *thread) {
  assert(NULL != thread);
  return thread->allocator;
}

size_t thread_get_iteration(thread_t *thread) {
  assert(NULL != thread);
  return atomic_load(&thread->iteration);
}

thread_state_t thread_get_current_state(thread_t *thread) {
  assert(NULL != thread);
  thread_state_t state;
  pthread_mutex_lock(&thread->lock);
  {
    state = thread->state;
  }
  pthread_mutex_unlock(&thread->lock);
  return state;
}

char *thread_get_name(thread_t *thread, char *out_items, size_t *out_count) {
  assert(NULL != thread);
  pthread_mutex_lock(&thread->lock);
  {
    if (out_items) {
      memcpy(out_items, thread->name.data, thread->name.count);
      out_items[thread->name.count] = '\0';
    }

    if (out_count) {
      *out_count = thread->name.count;
    }
  }
  pthread_mutex_unlock(&thread->lock);
  return out_items;
}

void thread_set_name(thread_t *thread, string_view_t name) {
  assert(NULL != thread);
  pthread_mutex_lock(&thread->lock);
  {
    size_t length = name.count > THREAD_MAX_NAME_LENGTH ? THREAD_MAX_NAME_LENGTH
                                                        : name.count;
    memcpy(thread->name.data, name.items, length);
    thread->name.data[length] = '\0';
    thread->name.count = length;
  }
  pthread_mutex_unlock(&thread->lock);
}

void thread_at_exit(thread_t *thread, void (*cleanup)(void *)) {
  assert(NULL != thread);
  pthread_mutex_lock(&thread->lock);
  {
    thread->at_exit = cleanup;
  }
  pthread_mutex_unlock(&thread->lock);
}

void thread_at_init(thread_t *thread, void (*init)(void *)) {
  assert(NULL != thread);
  pthread_mutex_lock(&thread->lock);
  {
    thread->at_init = init;
  }
  pthread_mutex_unlock(&thread->lock);
}

static thread_state_t thread_suspend(thread_t *thread) {
  thread_state_t successor_state = THREAD_STATE_SUSPENDED;
  pthread_mutex_lock(&thread->lock);
  {
    while (thread->state == THREAD_STATE_SUSPENDED) {
      pthread_cond_broadcast(&thread->suspend_condition);
      pthread_cond_wait(&thread->resume_condition, &thread->lock);
    }
    successor_state = thread->state;
  }
  pthread_mutex_unlock(&thread->lock);
  return successor_state;
}

static thread_state_t thread_execute(thread_t *thread) {

  if (setjmp(thread->unwind_buffer) == 0) {
    atomic_fetch_add(&thread->iteration, 1);
    thread->run(thread->arg);
  }

  thread_state_t next_state = THREAD_STATE_SUSPENDED;
  pthread_mutex_lock(&thread->lock);
  {
    if (thread->state == THREAD_STATE_DETACHED) {
      next_state = THREAD_STATE_DETACHED;
    } else {
      next_state = THREAD_STATE_SUSPENDED;
      thread->state = THREAD_STATE_SUSPENDED;
    }
  }
  pthread_mutex_unlock(&thread->lock);
  return next_state;
}

static void thread_destroy(thread_t *thread) {

  if (thread->at_exit) {
    thread->at_exit(thread->arg);
  }

  pthread_mutex_destroy(&thread->lock);
  pthread_cond_destroy(&thread->resume_condition);
  pthread_cond_destroy(&thread->suspend_condition);

  allocator_release(thread->allocator, thread, sizeof(thread_t),
                    _Alignof(thread_t));
}

static void *thread_run(void *arg) {

  thread_t *thread = arg;
  thread_state_t next_state = thread_suspend(thread);

  void (*at_init)(void *);
  pthread_mutex_lock(&thread->lock);
  {
    this_thread_ = thread;
    at_init = thread->at_init;
  }
  pthread_mutex_unlock(&thread->lock);

  if (at_init) {
    at_init(thread->arg);
  }

  for (;;) {
    switch (next_state) {
    case THREAD_STATE_SUSPENDED:
      next_state = thread_suspend(thread);
      break;
    case THREAD_STATE_RUNNING:
      next_state = thread_execute(thread);
      break;
    case THREAD_STATE_DETACHED:
      goto destroy;
    }
  }

destroy:
  thread_destroy(thread);
  return NULL;
}

thread_t *thread_create(allocator_t *allocator, void (*run)(void *),
                        void *arg) {

  assert(NULL != run);

  thread_t *thread =
      allocator_new(allocator, sizeof(thread_t), _Alignof(thread_t));
  if (NULL == thread) {
    return NULL;
  }

  memset(thread, 0, sizeof(thread_t));

  thread->allocator = allocator;
  thread->run = run;
  thread->arg = arg;
  thread->state = THREAD_STATE_SUSPENDED;

  atomic_init(&thread->iteration, 0);

  pthread_mutex_init(&thread->lock, NULL);
  pthread_cond_init(&thread->resume_condition, NULL);
  pthread_cond_init(&thread->suspend_condition, NULL);

  if (pthread_create(&thread->impl, NULL, thread_run, thread) != 0) {
    allocator_release(allocator, thread, sizeof(thread_t), _Alignof(thread_t));
    return NULL;
  }

  return thread;
}

void thread_start(thread_t *thread) {
  assert(NULL != thread);
  assert(thread != this_thread_);
  pthread_mutex_lock(&thread->lock);
  {
    thread->state = THREAD_STATE_RUNNING;
    pthread_cond_signal(&thread->resume_condition);
  }
  pthread_mutex_unlock(&thread->lock);
}

void thread_detach(thread_t *thread) {
  assert(NULL != thread);
  assert(thread != this_thread_);
  pthread_mutex_lock(&thread->lock);
  {
    pthread_detach(thread->impl);

    thread->state = THREAD_STATE_DETACHED;
    pthread_cond_signal(&thread->resume_condition);
  }
  pthread_mutex_unlock(&thread->lock);
}

void thread_await(thread_t *thread) {
  assert(NULL != thread);
  assert(thread != this_thread_);
  pthread_mutex_lock(&thread->lock);
  {
    while (thread->state == THREAD_STATE_RUNNING) {
      pthread_cond_signal(&thread->resume_condition);
      pthread_cond_wait(&thread->suspend_condition, &thread->lock);
    }
  }
  pthread_mutex_unlock(&thread->lock);
}

void thread_join(thread_t *thread) {
  assert(NULL != thread);
  assert(thread != this_thread_);
  pthread_mutex_lock(&thread->lock);
  {
    thread->state = THREAD_STATE_DETACHED;
    pthread_cond_signal(&thread->resume_condition);
  }
  pthread_mutex_unlock(&thread->lock);

  // Join with the forked thread
  pthread_join(thread->impl, NULL);
}

void thread_exit(void) {
  assert(NULL != this_thread_);
  longjmp(this_thread_->unwind_buffer, 1);
}
