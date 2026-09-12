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
#include "promise.h"
#endif

#include <assert.h>
#include <pthread.h>
#include <string.h>

struct promise {
  future_status_t status;
  uint32_t ref_count;
  allocator_t *allocator;
  size_t value_size;
  pthread_mutex_t mutex;
  pthread_cond_t completed_condition;
};

static void promise_destroy(promise_t *promise) {

  pthread_mutex_destroy(&promise->mutex);
  pthread_cond_destroy(&promise->completed_condition);

  allocator_release(promise->allocator, promise,
                    sizeof(promise_t) + promise->value_size,
                    _Alignof(max_align_t));
}

promise_t *promise_create(allocator_t *allocator, size_t value_size) {
  promise_t *promise = allocator_new(allocator, sizeof(promise_t) + value_size,
                                     _Alignof(max_align_t));
  if (NULL == promise) {
    return NULL;
  }
  promise->ref_count = 1;
  promise->allocator = allocator;
  promise->value_size = value_size;
  promise->status = FUTURE_STATUS_PENDING;

  pthread_mutex_init(&promise->mutex, NULL);
  pthread_cond_init(&promise->completed_condition, NULL);

  return promise;
}

void promise_complete(promise_t *promise, const void *value) {
  assert(NULL != promise);
  assert(NULL != value);
  bool last = false;
  pthread_mutex_lock(&promise->mutex);
  {
    promise->status = FUTURE_STATUS_COMPLETED;
    memcpy(promise + 1, value, promise->value_size);

    last = (--promise->ref_count) == 0;
    if (!last) {
      pthread_cond_broadcast(&promise->completed_condition);
    }
  }
  pthread_mutex_unlock(&promise->mutex);

  if (last) {
    promise_destroy(promise);
  }
}

void promise_break(promise_t *promise) {
  assert(NULL != promise);
  bool last = false;
  pthread_mutex_lock(&promise->mutex);
  {
    promise->status = FUTURE_STATUS_BROKEN_PROMISE;

    last = (--promise->ref_count) == 0;
    if (!last) {
      pthread_cond_broadcast(&promise->completed_condition);
    }
  }
  pthread_mutex_unlock(&promise->mutex);

  if (last) {
    promise_destroy(promise);
  }
}

future_t *promise_get_future(promise_t *promise) {
  assert(NULL != promise);
  pthread_mutex_lock(&promise->mutex);
  {
    ++promise->ref_count;
  }
  pthread_mutex_unlock(&promise->mutex);
  return (future_t *)promise;
}

bool future_get(future_t *future, void *out_value) {
  assert(NULL != future);

  promise_t *promise = (promise_t *)future;
  bool last = false;
  future_status_t status;

  pthread_mutex_lock(&promise->mutex);
  {
    while (promise->status == FUTURE_STATUS_PENDING) {
      pthread_cond_wait(&promise->completed_condition, &promise->mutex);
    }
    status = promise->status;
    last = (--promise->ref_count) == 0;

    if (status == FUTURE_STATUS_COMPLETED && NULL != out_value) {
      memcpy(out_value, promise + 1, promise->value_size);
    }
  }
  pthread_mutex_unlock(&promise->mutex);

  if (last) {
    promise_destroy(promise);
  }

  return status == FUTURE_STATUS_COMPLETED;
}

future_status_t future_get_status(future_t *future) {
  assert(NULL != future);

  future_status_t status;

  promise_t *promise = (promise_t *)future;
  pthread_mutex_lock(&promise->mutex);
  {
    status = promise->status;
  }
  pthread_mutex_unlock(&promise->mutex);

  return status;
}
