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
 * @file allocator.h
 *
 * This file defines a generic allocator abstraction (allocator_t) that uses a
 * callback interface for managing memory allocations. It also defines a set of
 * allocators that implement this interface. Namely:
 *
 * - arena_allocator : Non-growing arena allocator implementation
 * - auto_free : Doubly linked free list for freeing many allocations at once
 * - debug_allocator : Allocator with an attached debug callback
 * - scratch_allocator : Temporary allocator for short-lived allocations
 */

#if !defined(ALLOCATOR_H_)
#define ALLOCATOR_H_

#include <stddef.h>
#include <stdint.h>

/**
 * Holds parameters that can be passed to allocator callbacks.
 */
typedef struct allocation {
  /**
   * Pointer to an existing allocation, or NULL.
   *
   * This field may only be NULL if old_size == 0
   */
  void *memory;

  /**
   * Field specifying the size of an existing allocation, or 0.
   */
  size_t old_size;

  /**
   * Field specifying the requested size of the new allocation.
   * If this size is 0, the allocator will instead free the allocated
   * block of memory.
   *
   * new_size has to be greater than old_size, or be equal to 0.
   */
  size_t new_size;

  /**
   * Memory alignment of the allocation.
   */
  size_t alignment;

} allocation_t;

struct allocator;

/**
 * Allocator callback interface.
 *
 * If allocation->new_size > 0 and allocation->old_size == 0:
 *
 *   This function should allocate a new block of at least allocation->new_size
 *   bytes, aligned to allocation->alignment bytes.
 *
 *   On failure to allocate the requested block of memory, it should return
 *   NULL.
 *
 * If allocation->new_size > allocation->old_size and allocation->old_size != 0:
 *
 *    This function should allocate a new block of at least allocation->new_size
 *    bytes, aligned to allocation->alignment bytes. The allocation may reuse
 *    the memory pointed to by allocation->memory, if not, it should free
 *    allocation->memory upon a successful allocation.
 *
 *    Should the function perform a new allocation, then the memory region
 *    defined by (allocation->memory, allocation->old_size) should be copied
 *    into the new block of memory.
 *
 *    Should the allocation fail, NULL is returned, and allocation->memory is
 *    not freed.
 *
 * If allocation->new_size == 0:
 *
 *    This function should free the memory pointed to by allocation->memory.
 *    If allocation->memory was NULL, it should do nothing.
 *
 *    Always returns NULL.
 *
 * Any other state of allocation is a broken invariant, and should be asserted
 * against.
 */
typedef void *(*allocator_function_t)(struct allocator *allocator,
                                      const allocation_t *allocation);

/**
 * Callback type used to report information about a memory allocation.
 */
typedef void (*allocation_debug_function_t)(const allocation_t *allocation,
                                            void *user);

typedef struct allocator allocator_t;

/**
 * Polymorphic allocator structure. Other allocators can be created by adding
 * a field of this structure as it's first member.
 */
struct allocator {
  /**
   * Callback used to manage the allocations.
   */
  allocator_function_t allocate;

  /**
   * Optional data parameter that can be utilized by allocator implementations.
   */
  uintptr_t data;
};

typedef struct debug_allocator debug_allocator_t;

/**
 * Allocator implementation that performs allocations using an upstream
 * allocator and reports each performed allocation to a callback.
 */
struct debug_allocator {

  /**
   * Allocator structure to be used in the allocation calls.
   */
  allocator_t base;

  /**
   * Upstream allocator that performs the actual allocations.
   */
  allocator_t *upstream;

  /**
   * Callback used to report each memory allocation.
   */
  allocation_debug_function_t callback;
};

/**
 * An allocator implementation that uses malloc/realloc/free.
 */
extern allocator_t *malloc_allocator;

/**
 * Same as malloc_allocator, but never returns NULL (calls exit() if the
 * allocation failed).
 */
extern allocator_t *xmalloc_allocator;

/**
 * Creates a new arena allocator.
 *
 * This function may call exit(int) if the buffer parameter was NULL, and
 * the call to allocate the arena buffer failed.
 *
 * @param buffer_size Size of the arena buffer, has to be at least 128 bytes.
 * @param buffer Pointer to an existing buffer, or NULL to allocate
 *               the arena using malloc(size_t)
 *
 * @return A pointer to the arena allocator, which has been
 *         constructed at the start of the arena buffer.
 */
allocator_t *arena_allocator(size_t buffer_size, void *buffer);

/**
 * Gets a per-thread allocated arena allocator that is reused at every
 * subsequent call to this function.
 *
 * That is, after calling this function, the returned allocator is only valid
 * until the next call to scratch_allocator. To explicity free the existing
 * scratch buffer in this thread, call scratch_allocator with min_size = 0.
 *
 * This function will call exit(int) if the scratch buffer allocation failed.
 *
 * @param min_size Size hint to use when deciding the buffer size.
 *
 * @return A pointer to the new arena allocator.
 */
allocator_t *scratch_allocator(size_t min_size);

/**
 * Detaches the current scratch allocator, making the allocator slot available
 * for a new scratch allocator instance.
 *
 * @return The old scratch allocator.
 */
allocator_t *scratch_allocator_pop(void);

/**
 * Restores an old scratch allocator instance, making it the active scratch
 * allocator.
 *
 * @param old_allocator The return value of a previous call to
 *                      scratch_allocator_pop.
 */
void scratch_allocator_restore(allocator_t *old_allocator);

/**
 * Creates an allocator that permits freeing all allocated blocks in one call.
 *
 * @param upstream Allocator used to allocate the actual memory.
 *
 * @return The new allocator, or NULL if allocating the allocator header failed.
 */
allocator_t *auto_free_allocator(allocator_t *upstream);

/**
 * Frees all allocated blocks that where allocated using the given auto free
 * allocator.
 *
 * @param allocator Pointer to an allocator that was created
 *                  using auto_free_allocator(allocator_t *).
 */
void auto_free(allocator_t *allocator);

/**
 * Frees everyting owned by an auto free allocator, including the allocator
 * itself.
 *
 * @param allocator Pointer to an allocator that was created
 *                  using auto_free_allocator(allocator_t *)
 */
void auto_free_destroy(allocator_t *allocator);

/**
 * Creates an allocator that reports allocations back to a callback.
 *
 * @param upstream Allocator for performing the memory allocations.
 * @param debug Callback allocations are reported to.
 * @param user Optional argument that will be passed back to the callback.
 */
debug_allocator_t debug_allocator(allocator_t *upstream,
                                  allocation_debug_function_t debug,
                                  void *user);

/**
 * Creates an allocator that counts the number of allocated/freed bytes.
 *
 * @param upstream Allocator for performing the memory allocations.
 * @param out_balance Pointer to a variable that receives the number of
 *                    allocated bytes.
 */
debug_allocator_t trace_free_balance_allocator(allocator_t *upstream,
                                               size_t *out_balance);

/**
 * Allocate a new block of memory.
 *
 * @param allocator Memory allocator that performs the allocation, or NULL to
 *                  use malloc(size_t).
 * @param size Requested size of the allocation.
 * @param alignment Alignment of the memory allocation.
 * @return A contiguous memory region of at least {@param size} bytes, or NULL.
 */
void *allocator_new(allocator_t *allocator, size_t size, size_t alignment);

/**
 * Resize an existing allocation, or transfer the allocation into a larger block
 * of memory.
 *
 * Calling this functions is effectively equivalent to manually invoking the
 * allocator callback:
 *
 * ```c
 * allocator->allocate(allocator, &(allocation_t){
 *  .memory = hint,
 *  .old_size = old_size,
 *  .new_size = new_size,
 *  .alignment = alignment,
 * })
 * ```
 *
 * @param allocator Allocator that performs the allocation, or NULL to use
 *                  realloc(void *, size_t).
 * @param hint Pointer to an existing block of memory. May be NULL if old_size
 *             is 0.
 * @param old_size Size of the existing allocation, or 0.
 * @param new_size Number of bytes of the new allocation. This may either be 0,
 *                 in which case this function will act as allocator_resize, or
 *                 a number that is greater than old_size.
 * @return Either a pointer to a contiguous block of memory that holds at least
 *         new_size bytes, or NULL.
 */
void *allocator_resize(allocator_t *allocator, void *hint, size_t old_size,
                       size_t new_size, size_t alignment);

/**
 * Releases a block of memory that was previously allocated using the provided
 * allocator.
 *
 * @param allocator Allocator used to free the memory block, or NULL to use
 *                  free(void *).
 * @param memory A pointer that was returned by a previous call to the memory
 *               allocator.
 * @param size The value of the new_size parameter that was passed to the
 *             previous call to this memory allocator.
 * @param alignment The value of the alignment parameter that was passed to the
 *        previous call to this memory allocator.
 */
void allocator_release(allocator_t *allocator, void *memory, size_t size,
                       size_t alignment);

#if defined(UNITY_BUILD)
#include "allocator.c"
#include "arena_allocator.c"
#include "auto_free.c"
#include "scratch_allocator.c"
#endif

#endif // !defined(ALLOCATOR_H_)
