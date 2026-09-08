#if !defined(PROCESS_H_)
#define PROCESS_H_

#include "allocator.h"
#include "string_view.h"

#include <stdbool.h>
#include <stdio.h>

typedef enum process_file_usage {

  /**
   * Inherit the file handle from the running process
   */
  PROCESS_FILE_INHERIT = 0,

  /**
   * Assign the file handle to a pipe
   */
  PROCESS_FILE_PIPE = 1,

  /**
   * Direct the file handle to /dev/null
   */
  PROCESS_FILE_DEVNULL = 2,

  /**
   * Specify a file handle to use
   */
  PROCESS_FILE_SET = 3,

} process_file_usage_t;

typedef struct process_file process_file_t;
struct process_file {

  /**
   * Flags that specify how the handle is processed.
   */
  process_file_usage_t usage;

  /**
   * File handle to use.
   */
  FILE *handle;
};

typedef struct process_arguments process_arguments_t;
struct process_arguments {

  /**
   * Path to the executable.
   */
  string_view_t executable;

  /**
   * Command line arguments to pass to the new process.
   */
  size_t argument_count;
  string_view_t const *arguments;

  /**
   * Optional allocator argument.
   *
   * If specified, the process_* functions will use this allocator for memory
   * allocations. If NULL, malloc_allocator will be used.
   */
  allocator_t *allocator;

  /**
   * Specifies actions to redirect stdout, stderr and/or stdin
   */
  process_file_t out;
  process_file_t err;
  process_file_t in;

  /**
   * Optional user-defined callback that can be used to override the default
   * stream transfer behaviour.
   *
   * If specified, the callback should asynchronously transfer all bytes from
   * the stream specified by `in` and `in_handle` into the stream specified by
   * `out` and `out_handle`.
   *
   * @param in Function for getting the next byte from the input stream.
   * @param in_handle Pointer argument to pass to the in callback.
   * @param out Function for putting the next byte to the output stream.
   * @param out_handle Pointer argument to pass to the out callback.
   * @param user User-defined argument.
   *
   * @return true on success.
   */
  bool (*stream_callback)(int (*in)(void *), void *in_handle,
                          int (*out)(int, void *), void *out_handle,
                          void *user);
  void *user;
};

typedef struct process process_t;
typedef struct process_wait_result process_wait_result_t;

/**
 * Start a new process using the specified arguments.
 *
 * @param arguments Startup arguments to pass to the process.
 *
 * @return A handle to the newly created process, or NULL on an error.
 */
process_t *process_launch(process_arguments_t const *arguments);

/**
 * Marks the process as detached. That is, the resources owned by the handle are
 * released, and the process is marked as waited-for.
 *
 * @param process Process to detach.
 */
void process_detatch(process_t *process);

/**
 * Blocks execution of the running thread until the process terminates.
 *
 * @param process Process to wait for.
 * @param[out] out_result Pointer to a variable that receives the result handle.
 *
 * @return true on success.
 */
bool process_wait(process_t *process, process_wait_result_t **out_result);

/**
 * Blocks execution until all processes in the provided array have exited.
 *
 * If the results array contains a valid result from a process that terminated,
 * the corresponding process in the processes array will not be waited for.
 *
 * @param count Number of processes to wait for.
 * @param[in] processes Processes to wait for.
 * @param[inout] out_results A zero-initialized array of size count the wait
 *                           results will be written to.
 *
 * @return true if all processes have been waited for, false if there was an
 *         error.
 */
bool process_wait_all(size_t count, process_t *const *processes,
                      process_wait_result_t **out_results);

/**
 * Blocks execution until any of the given processes in the provided array have
 * exited.
 *
 * If the results array contains a valid result from a process that terminated,
 * the corresponding process in the processes array will not be waited for.
 *
 * @param count Number of processes to wait for.
 * @param[in] processes Processes to wait for.
 * @param[inout] out_results A zero-initialized array of size count the wait
 *                           results will be written to.
 *
 * @return The number of results that where produced.
 */
size_t process_wait_any(size_t count, process_t *const *processes,
                        process_wait_result_t **out_results);

/**
 * Checks if the process has existed.
 *
 * @param process Process to get the status of.
 * @param[out] out_result Pointer to a variable that receives the process
 *                        result.
 *
 * @return true if the process exited.
 */
bool process_poll(process_t *process, process_wait_result_t **out_result);

/**
 * Gets the exit code of a process that terminated.
 */
int process_get_exit_code(process_wait_result_t const *result);

/**
 * Gets the captured stdout of a terminated process.
 */
string_view_t process_get_stdout(process_wait_result_t const *result);

/**
 * Gets the captured stderr of a terminated process.
 */
string_view_t process_get_stderr(process_wait_result_t const *result);

/**
 * Deallocates the object holding the await result of a terminated process.
 */
void process_result_destroy(process_wait_result_t *result);

#endif
