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
#include "sync.h"
#endif

#include "thread.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define TIME_SECOND_IN_MILLISECONDS 1000
#define TIME_SECOND_IN_MICROSECONDS (1000 * TIME_SECOND_IN_MILLISECONDS)
#define TIME_SECOND_IN_NANOSECONDS (1000 * TIME_SECOND_IN_MICROSECONDS)

typedef enum mutex_state {
  MUTEX_STATE_LOCKED,
  MUTEX_STATE_UNLOCKED,
} mutex_state_t;

struct mutex {
  allocator_t *allocator;
  thread_t *holder;
  mutex_state_t state;
  pthread_mutex_t impl;
};

struct condition_variable {
  allocator_t *allocator;
  pthread_cond_t impl;
};

struct semaphore {
  allocator_t *allocator;
  size_t use_count;
  pthread_mutex_t mutex;
  pthread_cond_t condition;
};

struct fence {
  allocator_t *allocator;
  fence_state_t state;
  pthread_mutex_t mutex;
  pthread_cond_t condition;
};

mutex_t *mutex_create(allocator_t *allocator) {

  mutex_t *mutex = allocator_new(allocator, sizeof(mutex_t), _Alignof(mutex_t));
  if (NULL == mutex) {
    return NULL;
  }
  mutex->holder = NULL;
  mutex->allocator = allocator;
  mutex->state = MUTEX_STATE_UNLOCKED;

  int result = pthread_mutex_init(&mutex->impl, NULL);
  if (result != 0) {
    perror("pthread_mutex_init");
    abort();
  }
  return mutex;
}

void mutex_destroy(mutex_t *mutex) {

  if (NULL == mutex) {
    return;
  }
  assert(mutex->state == MUTEX_STATE_UNLOCKED);

  int result = pthread_mutex_destroy(&mutex->impl);
  if (result != 0) {
    perror("pthread_mutex_destroy");
    abort();
  }

  allocator_release(mutex->allocator, mutex, sizeof(mutex_t),
                    _Alignof(mutex_t));
}

void mutex_lock(mutex_t *mutex) {
  assert(NULL != mutex);

  thread_t *self = thread_get_this();
  assert(NULL == mutex->holder || mutex->holder != self);

  int result = pthread_mutex_lock(&mutex->impl);
  if (result != 0) {
    perror("pthread_mutex_lock");
    abort();
  }
  mutex->state = MUTEX_STATE_LOCKED;
  mutex->holder = self;
}

bool mutex_try_lock(mutex_t *mutex) {
  assert(NULL != mutex);

  thread_t *self = thread_get_this();
  assert(NULL == mutex->holder || mutex->holder != self);

  int result = pthread_mutex_trylock(&mutex->impl);
  if (result == EBUSY) {
    return false;
  } else if (result != 0) {
    perror("pthread_mutex_trylock");
    abort();
  }
  mutex->holder = self;
  mutex->state = MUTEX_STATE_LOCKED;
  return true;
}

void mutex_unlock(mutex_t *mutex) {
  assert(NULL != mutex);
  assert(mutex->state == MUTEX_STATE_LOCKED);
  assert(mutex->holder == thread_get_this());

  mutex->state = MUTEX_STATE_UNLOCKED;
  int result = pthread_mutex_unlock(&mutex->impl);
  if (result != 0) {
    perror("pthread_mutex_unlock");
    abort();
  }
}

condition_variable_t *condition_variable_create(allocator_t *allocator) {

  condition_variable_t *condition_variable = allocator_new(
      allocator, sizeof(condition_variable_t), _Alignof(condition_variable_t));
  if (NULL == condition_variable) {
    return condition_variable;
  }

  condition_variable->allocator = allocator;

  int result = pthread_cond_init(&condition_variable->impl, NULL);
  if (result != 0) {
    perror("pthread_cond_init");
    abort();
  }
  return condition_variable;
}

void condition_variable_destroy(condition_variable_t *condition_variable) {

  if (NULL == condition_variable) {
    return;
  }

  int result = pthread_cond_destroy(&condition_variable->impl);
  if (result != 0) {
    perror("pthread_cond_destroy");
    abort();
  }
  allocator_release(condition_variable->allocator, condition_variable,
                    sizeof(condition_variable_t),
                    _Alignof(condition_variable_t));
}

void condition_variable_wait(condition_variable_t *condition_variable,
                             mutex_t *mutex) {
  assert(NULL != condition_variable);
  assert(NULL != mutex);
  assert(mutex->holder == thread_get_this());

  int result = pthread_cond_wait(&condition_variable->impl, &mutex->impl);
  if (result != 0) {
    perror("pthread_cond_wait");
    abort();
  }
}

void condition_variable_signal(condition_variable_t *condition_variable) {

  assert(NULL != condition_variable);

  int result = pthread_cond_signal(&condition_variable->impl);
  if (result != 0) {
    perror("pthread_cond_signal");
    abort();
  }
}

void condition_variable_broadcast(condition_variable_t *condition_variable) {

  assert(NULL != condition_variable);

  int result = pthread_cond_broadcast(&condition_variable->impl);
  if (result != 0) {
    perror("pthread_cond_broadcast");
    abort();
  }
}

semaphore_t *semaphore_create(allocator_t *allocator, size_t initial_count) {

  semaphore_t *semaphore =
      allocator_new(allocator, sizeof(semaphore_t), _Alignof(semaphore_t));
  if (NULL == semaphore) {
    return NULL;
  }
  semaphore->allocator = allocator;
  semaphore->use_count = initial_count;

  int result = pthread_mutex_init(&semaphore->mutex, NULL);
  if (result != 0) {
    perror("pthread_mutex_init");
    abort();
  }

  result = pthread_cond_init(&semaphore->condition, NULL);
  if (result != 0) {
    perror("pthread_cond_init");
    abort();
  }
  return semaphore;
}

void semaphore_destroy(semaphore_t *semaphore) {

  if (NULL == semaphore) {
    return;
  }

  int result = pthread_mutex_destroy(&semaphore->mutex);
  if (result != 0) {
    perror("pthread_mutex_destroy");
    abort();
  }

  result = pthread_cond_destroy(&semaphore->condition);
  if (result != 0) {
    perror("pthread_cond_destroy");
    abort();
  }

  allocator_release(semaphore->allocator, semaphore, sizeof(semaphore_t),
                    _Alignof(semaphore_t));
}

void semaphore_release(semaphore_t *semaphore) {
  assert(NULL != semaphore);

  int result = pthread_mutex_lock(&semaphore->mutex);
  if (result != 0) {
    perror("pthread_mutex_lock");
    abort();
  }

  if (++semaphore->use_count == 1) {
    result = pthread_cond_signal(&semaphore->condition);
    if (result != 0) {
      perror("pthread_cond_signal");
      abort();
    }
  }

  result = pthread_mutex_unlock(&semaphore->mutex);
  if (result != 0) {
    perror("pthread_mutex_unlock");
    abort();
  }
}

void semaphore_acquire(semaphore_t *semaphore) {
  assert(NULL != semaphore);

  int result = pthread_mutex_lock(&semaphore->mutex);
  if (result != 0) {
    perror("pthread_mutex_lock");
    abort();
  }

  while (semaphore->use_count == 0) {
    result = pthread_cond_wait(&semaphore->condition, &semaphore->mutex);
    if (result != 0) {
      perror("pthread_cond_wait");
      abort();
    }
  }
  --semaphore->use_count;

  result = pthread_mutex_unlock(&semaphore->mutex);
  if (result != 0) {
    perror("pthread_mutex_unlock");
    abort();
  }
}

bool semaphore_try_acquire(semaphore_t *semaphore) {
  assert(NULL != semaphore);

  bool return_value = false;
  int result = pthread_mutex_lock(&semaphore->mutex);
  if (result != 0) {
    perror("pthread_mutex_lock");
    abort();
  }

  if (semaphore->use_count > 0) {
    --semaphore->use_count;
    return_value = true;
  }

  result = pthread_mutex_unlock(&semaphore->mutex);
  if (result != 0) {
    perror("pthread_mutex_unlock");
    abort();
  }
  return return_value;
}

fence_t *fence_create(allocator_t *allocator, fence_state_t initial_state) {

  fence_t *fence = allocator_new(allocator, sizeof(fence_t), _Alignof(fence_t));
  if (NULL == fence) {
    return NULL;
  }
  fence->allocator = allocator;
  fence->state = initial_state;

  int result = pthread_mutex_init(&fence->mutex, NULL);
  if (result != 0) {
    perror("pthread_mutex_init");
    abort();
  }

  result = pthread_cond_init(&fence->condition, NULL);
  if (result != 0) {
    perror("pthread_cond_init");
    abort();
  }
  return fence;
}

void fence_destroy(fence_t *fence) {
  if (NULL == fence) {
    return;
  }

  int result = pthread_mutex_destroy(&fence->mutex);
  if (result != 0) {
    perror("pthread_mutex_destroy");
    abort();
  }

  result = pthread_cond_destroy(&fence->condition);
  if (result != 0) {
    perror("pthread_cond_destroy");
    abort();
  }

  allocator_release(fence->allocator, fence, sizeof(fence_t),
                    _Alignof(fence_t));
}

void fence_wait(fence_t *fence) {

  assert(NULL != fence);

  int result = pthread_mutex_lock(&fence->mutex);
  if (result != 0) {
    perror("pthread_mutex_lock");
    abort();
  }

  while (fence->state != FENCE_STATE_SIGNALED) {
    result = pthread_cond_wait(&fence->condition, &fence->mutex);
    if (result != 0) {
      perror("pthread_cond_wait");
      abort();
    }
  }

  result = pthread_mutex_unlock(&fence->mutex);
  if (result != 0) {
    perror("pthread_mutex_unlock");
    abort();
  }
}

bool fence_try_wait(fence_t *fence) {
  assert(NULL != fence);

  bool return_value = false;
  int result = pthread_mutex_lock(&fence->mutex);
  if (result != 0) {
    perror("pthread_mutex_lock");
    abort();
  }

  if (fence->state == FENCE_STATE_SIGNALED) {
    return_value = true;
  }

  result = pthread_mutex_unlock(&fence->mutex);
  if (result != 0) {
    perror("pthread_mutex_unlock");
    abort();
  }
  return return_value;
}

void fence_signal(fence_t *fence) {
  assert(NULL != fence);
  assert(fence->state == FENCE_STATE_CLOSED);

  int result = pthread_mutex_lock(&fence->mutex);
  if (result != 0) {
    perror("pthread_mutex_lock");
    abort();
  }

  fence->state = FENCE_STATE_SIGNALED;
  result = pthread_cond_broadcast(&fence->condition);
  if (result != 0) {
    perror("pthread_cond_broadcast");
    abort();
  }

  result = pthread_mutex_unlock(&fence->mutex);
  if (result != 0) {
    perror("pthread_mutex_unlock");
    abort();
  }
}

void fence_reset(fence_t *fence) {
  assert(NULL != fence);
  assert(fence->state == FENCE_STATE_SIGNALED);

  int result = pthread_mutex_lock(&fence->mutex);
  if (result != 0) {
    perror("pthread_mutex_lock");
    abort();
  }

  fence->state = FENCE_STATE_CLOSED;

  result = pthread_mutex_unlock(&fence->mutex);
  if (result != 0) {
    perror("pthread_mutex_unlock");
    abort();
  }
}
