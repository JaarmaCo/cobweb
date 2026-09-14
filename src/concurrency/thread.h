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
 * @file concurrency/thread.h
 *
 * A resumable thread implementation based on POSIX threads.
 *
 * This thread system implements a thread with three possible states:
 *
 * - Suspended : The thread is waiting for a signal from it's parent.
 * - Executing : The thread is running it's assigned entrypoint function.
 * - Detached : The thread is scheduled for destruction.
 *
 * NOTE: A "parent" is the thread that created the running thread
 *
 * Threads are by default created in the suspended state, which allows
 * the parent to assign attributes to the thread before it starts executing.
 *
 * Once the thread has executed, it will revert back into the suspended state,
 * allowing the parent to either restart it, or terminate it by assigning it as
 * detached.
 */

#if !defined(CONCURRENCY_THREAD_H_)
#define CONCURRENCY_THREAD_H_

#include <stdbool.h>

#include "../allocator.h"
#include "../string_view.h"

/**
 * Maximum length that can be assigned to the name attribute of a thread.
 */
#define THREAD_MAX_NAME_LENGTH 255

/**
 * Enumerates the different states of a thread.
 */
typedef enum thread_state {

  /**
   * Thread is waiting for a control signal from it's parent thread.
   */
  THREAD_STATE_SUSPENDED,

  /**
   * Thread is executing it's entrypoint function.
   */
  THREAD_STATE_RUNNING,

  /**
   * Thread is no longer attached to it's parent and will be destroyed when the
   * entrypoint returns.
   */
  THREAD_STATE_DETACHED,

} thread_state_t;

/**
 * Opaque handle to a running thread.
 */
typedef struct thread thread_t;

/**
 * Gets a pointer to the running thread.
 *
 * @return A thread created by thread_create, or NULL if this thread was not
 *         created by thread_create.
 */
thread_t *thread_get_this(void);

/**
 * Gets the allocator that was used to create the thread.
 */
allocator_t *thread_get_allocator(thread_t *thread);

/**
 * Gets the current state of execution of the given thread.
 */
thread_state_t thread_get_current_state(thread_t *thread);

/**
 * Gets the current iteration number (number of times the entrypoint was called)
 * of the specified thread.
 *
 * @param thread Thread to get the iteration of.
 *
 * @return The number of entrypoint calls that have been made to the specified
 *         thread.
 */
size_t thread_get_iteration(thread_t *thread);

/**
 * Gets the name that was assigned to the given thread.
 *
 * The name may never be longer than THREAD_MAX_NAME_LENGTH,
 * so a buffer of size THREAD_MAX_NAME_LENGTH + 1 is always sufficient for
 * storing the thread name.
 *
 * @param thread The thread to get the name attribute of.
 * @param[out] out_items Pointer to an array of characters that receives the
 *                       thread name (or NULL).
 * @param[out] out_count Pointer to a variable of type size_t that receives the
 *                       length of the name (or NULL).
 * @return The value of out_items
 */
char *thread_get_name(thread_t *thread, char *out_items, size_t *out_count);

/**
 * Sets the name attribute of the given thread.
 *
 * @param thread Thread to assign a name to.
 * @param name Value to assign.
 */
void thread_set_name(thread_t *thread, string_view_t name);

/**
 * Creates a new thread with an entry point function. The thread will be in
 * waiting state when initially created.
 *
 * After creating the thread, it has to be started by thread_start to actually
 * begin running. Calling thread_detach, or thread_join with a thread that
 * hasn't started will cause the thread to immediately exit instead of calling
 * the entry point function.
 *
 * @param allocator Allocator used to allocate the thread handle.
 * @param run Start procedure of the thread.
 * @param arg Argument to pass to the start routine.
 */
thread_t *thread_create(allocator_t *allocator, void (*run)(void *), void *arg);

/**
 * Signal to the thread that it may begin executing the entrypoint function.
 *
 * If the thread exited, calling this function will cause the thread to revert
 * to the started state; causing the entrypoint to begin executing again.
 *
 * thread_start should not be called on a thread handle that was invalidated
 * by a call to thread_join or thread_detach.
 *
 * @param thread Thread to start.
 */
void thread_start(thread_t *thread);

/**
 * Registers a callback that will be invoked when the thread exits.
 *
 * @param thread Thread to register a cleanup callback for.
 * @param cleanup Callback that accepts the argument that was passed to the
 *                thread in thread_create.
 */
void thread_at_exit(thread_t *thread, void (*cleanup)(void *));

/**
 * Registers a callback that will be invoked before the first execution of the
 * thread entry point.
 *
 * @param thread thread to register a callback for.
 * @param cleanup Callback that accepts the argument that was passed to the
 *                thread in thread_create.
 */
void thread_at_init(thread_t *thread, void (*init)(void *));

/**
 * Signals to the thread that it will not be joined, and that it should
 * therefore free itself when it exits.
 */
void thread_detach(thread_t *thread);

/**
 * Waits for the specified thread to enter suspension.
 *
 * @param thread Thread to wait for.
 */
void thread_await(thread_t *thread);

/**
 * Detaches the thread, and then waits for it to exit.
 */
void thread_join(thread_t *thread);

/**
 * When called inside a running thread; unwinds the stack and reverts the
 * running thread back to the suspended state.
 */
_Noreturn void thread_exit(void);

/**
 * Suspend thread execution until a new thread_start request is received.
 */
void thread_suspend(void);

#endif

#if defined(UNITY_BUILD)
#include "concurrency/thread.c"
#endif
