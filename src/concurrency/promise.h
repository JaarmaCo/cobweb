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

/**
 * @file concurrency/promise.h
 *
 * This header defines a shared promise/future type.
 */

#if !defined(CONCURRENCY_PROMISE_H_)
#define CONCURRENCY_PROMISE_H_

#include <stdbool.h>

#include "../allocator.h"

/**
 * Enumerates the possible states of a future object.
 */
typedef enum future_status {

  /**
   * The future promise has not yet been completed.
   */
  FUTURE_STATUS_PENDING,

  /**
   * The future promise completed, and supplied a value.
   */
  FUTURE_STATUS_COMPLETED,

  /**
   * The future promise completed, but did not supply a value.
   */
  FUTURE_STATUS_BROKEN_PROMISE,

} future_status_t;

typedef struct promise promise_t;
typedef struct future future_t;

/**
 * Creates a new promise object.
 *
 * @param allocator Allocator used to allocate the promise.
 * @param value_size Size of the promise value type.
 *
 * @return The new promise, or NULL if an error occured.
 */
promise_t *promise_create(allocator_t *allocator, size_t value_size);

/**
 * Complete a promise by supplying a value.
 *
 * The promise object is released by calling this function, and should not be
 * interacted with again (except through a future_t handle).
 *
 * If there where no waiting futures, the promise/future state is destroyed.
 *
 * @param promise Promise to complete.
 * @param[in] value Pointer to the value used to complete the promise.
 */
void promise_complete(promise_t *promise, const void *value);

/**
 * Marks a promise as broken. That is, completes the promise, but does not
 * supply a value.
 *
 * The promise object is released by calling this function, and should not be
 * interacted with again (except through a future_t handle).
 *
 * If there where no waiting futures, the promise/future state is destroyed.
 *
 * @param promise Promise to break.
 */
void promise_break(promise_t *promise);

/**
 * Gets a promise future_t handle, which can be used to extract the promised
 * value.
 *
 * It is possible to extract multiple futures from one promise, in which case,
 * the last call to future_get will free the promise/future state.
 *
 * @param promise Promise to get a future from.
 *
 * @return A pointer to a future that can be used to extract the promised value.
 */
future_t *promise_get_future(promise_t *promise);

/**
 * Blocks the execution of the calling thread until the given future is
 * completed, and then returns the future result.
 *
 * The future is released after calling this function, and should not be
 * interacted with again.
 *
 * @param future Future to wait for.
 * @param[out] out_value Pointer to a variable that receives the promised value,
 *                       or NULL to ignore the value.
 *
 * @return true if the value was supplied, false if the promise was broken.
 */
bool future_get(future_t *future, void *out_value);

/**
 * Gets the current status of the given future.
 */
future_status_t future_get_status(future_t *future);

#endif

#if defined(UNITY_BUILD)
#include "promise.c"
#endif
