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
 * @file concurrency/linked_queue.h
 *
 * This header template file contains a lock-free concurrent linked queue
 * implementation.
 *
 * The available parameters are:
 *
 * TYPE_0 : Linked queue structure type (optional)
 * TYPE_1 : Linked queue node type (optional)
 * TYPE_2 : Value type (required)
 * FUNCTION_0 : Value cleanup function (optional)
 * PREFIX : Prefix to prepend to the generated symbols (optional)
 * SUFFIX : Suffix to append to the generated symbols (optional)
 */

#if !defined(C_SOURCE) && !defined(C_HEADER) && !defined(HEADER_ONLY)
#define TYPE_2 int
#define SUFFIX _i
#define PREFIX concurrent_linked_queue_
#define HEADER_ONLY
#endif

#define REQUIRE_TYPE_2
#include "../template-def.h"

#if defined(C_HEADER)

#include "../allocator.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>

#if !defined(TYPE_0)
#define TYPE_0 M_CAT(concurrent_linked_queue, SUFFIX)
#endif

#if !defined(TYPE_1)
#define TYPE_1 M_CAT(concurrent_linked_queue_node, SUFFIX)
#endif

#define T TYPE_2

#define concurrent_linked_queue TYPE_0
#define concurrent_linked_queue_node TYPE_1
#define enqueue M_NAME(enqueue)
#define dequeue M_NAME(dequeue)
#define destroy M_NAME(destroy)
#define create M_NAME(create)

#if defined(DEFINE_STRUCT)

/**
 * Singly linked queue node type.
 */
typedef struct concurrent_linked_queue_node {

  /**
   * Stores the value of this node.
   */
  T item;

  /**
   * Atomic pointer to the next element.
   */
  struct concurrent_linked_queue_node *_Atomic next;

} concurrent_linked_queue_node;

/**
 * The queue structure.
 */
typedef struct concurrent_linked_queue {

  /**
   * Allocator used to allocate and release nodes (should be thread-safe).
   */
  allocator_t *allocator;

  /**
   * Pointer to a dummy node, head->next is the actual first element.
   */
  concurrent_linked_queue_node *_Atomic head;

  /**
   * Pointer to the last queue node, or a pointer to head if the queue is empty.
   */
  concurrent_linked_queue_node *_Atomic tail;

} concurrent_linked_queue;

#endif

/**
 * Allocate the initial dummy node.
 */
INLINE
bool create(concurrent_linked_queue *queue, allocator_t *allocator);

/**
 * Free the memory used by the provided queue.
 */
INLINE
void destroy(concurrent_linked_queue *queue);

/**
 * Put a value into the tail of the queue.
 *
 * @param queue Queue to modify.
 * @param item Value to insert.
 *
 * @return true on success, false if a memory allocation failed.
 */
INLINE
bool enqueue(concurrent_linked_queue *queue, T item);

/**
 * Pop a value of the queue.
 *
 * @param queue Queue to modify.
 * @param[out] out_item Pointer to a variable to copy the queue item into, or
 *                      NULL to ignore the queue item.
 *
 * @return true if the item was retrieved, false if the queue was empty.
 */
INLINE
bool dequeue(concurrent_linked_queue *queue, T *out_item);

#endif

#if defined(C_SOURCE)

INLINE
bool create(concurrent_linked_queue *queue, allocator_t *allocator) {
  assert(NULL != queue);
  concurrent_linked_queue_node *head =
      allocator_new(allocator, sizeof(concurrent_linked_queue_node),
                    _Alignof(concurrent_linked_queue_node));
  if (NULL == head) {
    return false;
  }
  head->next = NULL;

  queue->allocator = allocator;
  queue->tail = head;
  queue->head = head;
  return true;
}

INLINE
void destroy(concurrent_linked_queue *queue) {

  if (NULL == queue) {
    return;
  }

  concurrent_linked_queue_node *node = queue->head;
  while (node) {
    concurrent_linked_queue_node *next = node->next;

#if defined(FUNCTION_0)
    if (node != queue->head) {
      FUNCTION_0(node->item);
    }
#endif

    allocator_release(queue->allocator, node,
                      sizeof(concurrent_linked_queue_node),
                      _Alignof(concurrent_linked_queue_node));
    node = next;
  }

  queue->head = NULL;
  queue->tail = NULL;
}

INLINE
bool enqueue(concurrent_linked_queue *queue, T item) {
  assert(NULL != queue);

  concurrent_linked_queue_node *node =
      allocator_new(queue->allocator, sizeof(concurrent_linked_queue_node),
                    _Alignof(concurrent_linked_queue_node));
  if (NULL == node) {
    return false;
  }

  node->item = item;
  node->next = NULL;

  for (;;) {
    concurrent_linked_queue_node *tail = atomic_load(&queue->tail);
    concurrent_linked_queue_node *next = tail->next;

    if (tail != atomic_load(&queue->tail)) {
      continue;
    }

    if (next) {
      atomic_compare_exchange_strong(&queue->tail, &tail, node);
      continue;
    }

    if (atomic_compare_exchange_strong(&tail->next, &next, node)) {
      atomic_compare_exchange_strong(&queue->tail, &tail, node);
      break;
    }
  }
  return true;
}

INLINE
bool dequeue(concurrent_linked_queue *queue, T *out_item) {
  assert(NULL != queue);

  bool empty = false;
  for (;;) {
    concurrent_linked_queue_node *head = atomic_load(&queue->head);
    concurrent_linked_queue_node *tail = atomic_load(&queue->tail);
    concurrent_linked_queue_node *next = head->next;

    if (head != atomic_load(&queue->head)) {
      continue;
    }

    // "Head" is always a dummy node.
    // I.e if next is NULL, the queue must be empty.
    if (!next) {
      empty = true;
      break;
    }

    if (head == tail) {
      atomic_compare_exchange_strong(&queue->tail, &tail, next);
      continue;
    }

    // Detach the head and make the first element the new dummy node.
    //
    atomic_compare_exchange_strong(&queue->head, &head, next);
    if (out_item) {
      *out_item = next->item;
    }
    allocator_release(queue->allocator, head,
                      sizeof(concurrent_linked_queue_node),
                      _Alignof(concurrent_linked_queue_node));
    break;
  }
  return !empty;
}

#endif

#undef T
#undef concurrent_linked_queue
#undef concurrent_linked_queue_node
#undef destroy
#undef create
#undef enqueue
#undef dequeue
#include "../template-undef.h"
