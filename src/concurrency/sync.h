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
 * @file concurrency/sync.h
 *
 * This is a thin wrapper over the underlying synchronization promitives of the
 * platform/os (I.e pthread). The main purpose of this file is to provide a
 * platform and operating system agnostic api that can eventually be used to
 * target multiple systems. For now, this is POSIX-only.
 */

#if !defined(CONCURRENCY_SYNC_H_)
#define CONCURRENCY_SYNC_H_

#include <stdbool.h>
#include <time.h>

#include "../allocator.h"

/**
 * Enumerates the states a fence synchronization primitive can assume.
 */
typedef enum fence_state {

  /**
   * Fence is "signaled"; threads waiting on this fence will be allowed to
   * continue executing.
   */
  FENCE_STATE_SIGNALED,

  /**
   * Fence is "closed/blocked"; threads waiting on this fence will be blocked
   * until another thread puts the fence into the signaled state.
   */
  FENCE_STATE_CLOSED,

} fence_state_t;

/**
 * Synchronization primivive types.
 *
 * These have intentionally been made opaque to permit future expansion into
 * multi-platform builds.
 * @{
 */
typedef struct mutex mutex_t;
typedef struct condition_variable condition_variable_t;
typedef struct semaphore semaphore_t;
typedef struct fence fence_t;
/**
 * @}
 */

/**
 * Creates an instance of a synchronization primitive.
 *
 * @param allocator Allocator used to allocate the primivive
 * @param initial_count Initial count of a created semaphore
 * @param initial_state Whether a fence should be initially signaled
 *
 * @return A pointer to the primitive, or NULL if the memory allocation failed.
 * @{
 */
mutex_t *mutex_create(allocator_t *allocator);
condition_variable_t *condition_variable_create(allocator_t *allocator);
semaphore_t *semaphore_create(allocator_t *allocator, size_t initial_count);
fence_t *fence_create(allocator_t *allocator, fence_state_t initial_state);
/**
 * @}
 */

/**
 * Checks the sychronization primitive invariants then deallocates the object.
 * @{
 */
void mutex_destroy(mutex_t *mutex);
void condition_variable_destroy(condition_variable_t *condition_variable);
void semaphore_destroy(semaphore_t *semaphore);
void fence_destroy(fence_t *fence);
/**
 * @}
 */

/**
 * Lock, or attempt to lock a mutex.
 *
 * The "try" version of this function does not block, and returns either:
 *
 * - true if the mutex could be locked without blocking
 * - false if the mutex was already blocked
 * @{
 */
void mutex_lock(mutex_t *mutex);
bool mutex_try_lock(mutex_t *mutex);
/**
 * @}
 */

/**
 * Releases a lock held on a mutex.
 *
 * The owner of the mutex has to be the thread calling this function.
 */
void mutex_unlock(mutex_t *mutex);

/**
 * Waits for the condition variable to be signaled.
 *
 * The provided mutex has to be locked by the calling thread before attempting
 * to call this function. Once called, condition_variable_wait will unlock the
 * given mutex, and wait for a signal to arrive. Once a signal is present, the
 * mutex is again locked and condition_variable_wait returns.
 */
void condition_variable_wait(condition_variable_t *condition_variable,
                             mutex_t *mutex);

/**
 * Wakes up threads that are waiting for the given condition variable.
 *
 * The "broadcast" variant of this function wakes up all waiting threads, and
 * the "signal" version awakes only one waiting thread. These functions should
 * only be called if a lock is held on the mutex the awakened threads will lock
 * upon waking up. Failing to do so may cause a race condition.
 * @{
 */
void condition_variable_signal(condition_variable_t *condition_variable);
void condition_variable_broadcast(condition_variable_t *condition_variable);
/**
 * @}
 */

/**
 * Increments the count on the semaphore.
 *
 * Calling this function may wake up threads that are waiting on this semaphore,
 * if the semaphore count increased from 0 to 1 and there where blocked threads
 * waiting on it.
 */
void semaphore_release(semaphore_t *semaphore);

/**
 * Acquires, or attempts to acquire a semaphore by decrementing the count.
 *
 * If the semaphore count is 0 at the time of calling this function, the calling
 * thread will be blocked until another thread invokes semaphore_release. For
 * the "try" variant of this function, false will instead be returned, and the
 * semaphore counter remains unaltered.
 *
 * If the "try" variant of function returns true, then the semaphore has been
 * acquired, and the calling thread may proceed to use the resource being
 * guarded by this semaphore.
 * @{
 */
void semaphore_acquire(semaphore_t *semaphore);
bool semaphore_try_acquire(semaphore_t *semaphore);
/**
 * @}
 */

/**
 * Waits for, or attempts to wait for the given fence to enter the signaled
 * state.
 *
 * The "try" variant of this function will not block the calling thread if the
 * fence was not signaled, and will instead return false (and return true if
 * signaled).
 * @{
 */
void fence_wait(fence_t *fence);
bool fence_try_wait(fence_t *fence);
/**
 * @}
 */

/**
 * Puts the fence into the signaled state, and awakens any threads that are
 * waiting on this fence to change state.
 *
 * This function may only be called on a fence that is in the closed/blocked
 * state.
 */
void fence_signal(fence_t *fence);

/**
 * Puts the given fence from a signaled state into a closed/blocked state.
 */
void fence_reset(fence_t *fence);

#endif

#if defined(UNITY_BUILD)
#include "sync.c"
#endif
