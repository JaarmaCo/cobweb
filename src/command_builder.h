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
 * @file command_builder.h
 *
 * Declares utilities for building and executing command lines.
 *
 * The main type of interest in this file is command_builder_t, which is a
 * dynamic array implementation that stores entries of type char *.
 *
 * The command line is built from left to right, and may be manipulated through:
 *
 * - Appending string arguments
 * - Formatting using snprintf
 * - Expanding arguments containing environment variables
 * - Copying
 *
 * This file also contains utilities for both executing and waiting for executed
 * commands. As well as a few utilities for executing compilation-like commands.
 */

#if !defined(COMMAND_BUILDER_H_)
#define COMMAND_BUILDER_H_

#include "allocator.h"
#include "env.h"
#include "string_view.h"

/**
 * Macro for rebuilding the current source file (a.k.a "self").
 *
 * Using this macro requires the following macros to be defined:
 * - CC : Should expand to the name of the C compiler CLI
 * - CFLAGS : Should expand to a comma separated list of compile-flags that ends
 *            with NULL.
 * - LDFLAGS : Should expand to a comma separated list of linker-flags that ends
 *             with NULL.
 */
#define CMD_REBUILD_SELF() cmd_rebuild_self(__FILE__, CC, CFLAGS, LDFLAGS)

typedef struct command command_t;
typedef struct command_builder command_builder_t;

/**
 * A convenience type for constructing and executing command lines that can
 * later be executed.
 *
 * Because this is a utility designed for convenience, functions that operate on
 * command_builder_t will not handle errors and will also not free allocated
 * memory. Handling of these errors have instead been delegated to custom
 * callbacks (such as allocator_t) and the use of the exit(int) function.
 */
struct command_builder {

  /**
   * Allocator responsible for allocating memory for the command builder.
   *
   * command_builder does not support releasing allocated data, as it is
   * intended to either be used right before a call to exit(int), or using
   * allocators that can be destroyed.
   */
  allocator_t *allocator;

  /**
   * NULL terminated array of strings to pass to the subprocess.
   */
  char **items;

  /**
   * The value of argc for the new process.
   */
  size_t count;

  /**
   * Capacity of the argv array.
   */
  size_t capacity;

  /**
   * Optional environment that may be used in cmd_expand
   */
  env_t *env;

  /**
   * File to echo executed commands to.
   */
  FILE *echo_channel;

  /**
   * A callback that, if not NULL, is invoked whenever an error occurs.
   *
   * @param cmd The command builder where the error occured.
   * @param error_code The numeric code of the error.
   * @param error_message A human-readable error message.
   * @param user User-defined argument that is passed back to the callback.
   */
  void (*handle_error)(command_builder_t *cmd, int error_code,
                       string_view_t error_message, void *user);

  /**
   * An optional user-defined argument that is passed back to handle_error when
   * it is invoked.
   */
  void *handle_error_arg;
};

static inline string_view_t c_file_pattern(string_view_t filename) {
  size_t pos = sv_rfind_substr(filename, SV(".c"), 0);
  if (pos == (size_t)-1) {
    fprintf(stderr, "%.*s is not a C source file", (int)filename.count,
            filename.items);
    exit(1);
  }
  return sv_take(filename, pos);
}

_Noreturn void cmd_rebuild_self(const char *file, const char *cc, ...);

/**
 * Use the current command line as a template to compile the provided source
 * files.
 *
 * @param cmd Command line to use.
 * @param job_count Maximum number of concurrent compilations.
 * @param src_dir Directory where the source files are contained.
 * @param out_dir Directory to put the object files in.
 * @param va NULL-terminated variadic list of source file names (const char *).
 */
void cmd_compile(command_builder_t *cmd, int job_count, const char *src_dir,
                 const char *out_dir, ...);

/**
 * Use the current command line state as a template for building and running a
 * test C source file.
 *
 * @param cmd Command line to use.
 * @param test_dir Directory where the test is located.
 * @param out_dir Output directory to put intermediate files in.
 * @param test Name of the test source file.
 * @param va NULL-terminated variadic list of source file names (const char *).
 *           The source files in this list have to already have been compiled
 *           using cmd_compile.
 */
void cmd_run_test(command_builder_t *cmd, const char *test_dir,
                  const char *out_dir, const char *test, ...);

/**
 * Use the current state of the command builder to write a compile_flags.txt
 * file.
 */
void cmd_write_compilation_database(command_builder_t *cmd);

/**
 * Enables output to stderr on an error.
 */
void cmd_enable_error_output(command_builder_t *cmd);

/**
 * Ensure the provided directory exists.
 */
void cmd_ensure_directory(const char *dirname);

/**
 * Find an executable program using the command environment, and assign it as
 * argv[0].
 *
 * @return true if the executable was found.
 */
bool cmd_find_executable(command_builder_t *cmd, const char *name);

/**
 * Append a nul-terminated string to the command list.
 */
void cmd_append(command_builder_t *cmd, const char *entry);

/**
 * Append all variadic C-string arguments to the command builder.
 *
 * @param cmd Command builder to append to.
 * @param va NULL-terminated variadic list of C-string arguments.
 */
void cmd_append_all(command_builder_t *cmd, ...);

/**
 * Append a string view to the command list.
 */
void cmd_append_sv(command_builder_t *cmd, string_view_t sv);

/**
 * Append a formatted string to the command line (standard C-format).
 * @{
 */
void cmd_format(command_builder_t *cmd, const char *fmt, ...);
void cmd_vformat(command_builder_t *cmd, const char *fmt, va_list va);
/**
 * @}
 */

/**
 * Expands the given string using the assigned command builder environment
 * (env_t), if no environment was assigned, the string is simply appended.
 * @{
 */
void cmd_expand(command_builder_t *cmd, const char *pattern);
void cmd_expand_all(command_builder_t *cmd, ...);
/**
 * @}
 */

/**
 * Resets the command builder to it's initial state, allowing the object to be
 * reused for other command line constructions.
 */
void cmd_reset(command_builder_t *cmd);

/**
 * Copy a command builder to another.
 *
 * @param cmd Command builder to copy arguments to.
 * @param source Command builder to copy.
 */
void cmd_clone(command_builder_t *cmd, const command_builder_t *source);

/**
 * Set a file the command builder will echo executed commands to.
 */
void cmd_echo_to(command_builder_t *cmd, FILE *f);

/**
 * Executes the constructed command line by spawning a new process, and waiting
 * for it to exit.
 *
 * This function will raise an error if the requested executable did not exist.
 *
 * @return The exit code of the spawned process.
 */
int cmd_exec_sync(command_builder_t *cmd);

/**
 * Performs the same operations as cmd_exec_sync, but does not raise an error if
 * the executable did not exist.
 *
 * @return The exit code of the spawned process, or the value of errno if an
 *         error occured.
 */
int cmd_exec_sync_maybe(command_builder_t *cmd);

/**
 * Executes the constructed command line by spawning a new process. This
 * function does not wait for the process to exit, instead it returns a handle
 * that can be used to wait for the process at a later stage.
 *
 * @return An awaitable handle to the new process.
 */
command_t *cmd_exec_async(command_builder_t *cmd);

/**
 * Performs the same actions as cmd_exec_async, but does not raise an error if
 * the executable was not found.
 *
 * @return An awaitable handle to the new process, or NULL.
 */
command_t *cmd_exec_async_maybe(command_builder_t *cmd);

/**
 * Waits for an executed command to terminate.
 *
 * @param handle A handle to the process to wait for. This handle is destroyed
 *               after calling this function. If the handle was NULL, -1 is
 *               returned and no process is awaited.
 *
 * @return The exit code of the command.
 */
int cmd_wait(command_t *handle);

/**
 * Check if a command has exited. If it has, this function returns true,
 * the handle is destroyed, and the exit code is written out. Else false
 * is returned and the handle remains valid.
 *
 * If the handle is NULL, false is returned and exit_code is set to -1.
 *
 * @param handle Process handle to poll.
 * @param[out] exit_code Pointer to a variable that receives the exit status of
 *                       the process, or NULL to ignore the exit status.
 *
 * @return true if the process exited, false if not.
 */
bool cmd_poll(command_t *handle, int *exit_code);

/**
 * Waits for all the commands in the given array of handles to terminate.
 *
 * On success, the handles array is overwritten with NULL values.
 *
 * @param count Number of commands to wait for.
 * @param[inout] handles Array of commands to wait for.
 * @param[out] exit_codes Array where the exit codes will be stored (or NULL).
 */
void cmd_wait_every(size_t count, command_t **handles, int *exit_codes);

/**
 * Waits for one of the commands in the given array of handles to terminate.
 *
 * The waited for command in the handles array is set to NULL on success, thus
 * allowing for subsequent calls to cmd_wait_every with the same array.
 *
 * @param count Number of commands in the array.
 * @param[inout] handles Array of handles to wait for.
 * @param[out] exit_code Pointer to a variable that receives the exit code of
 *                       the terminated process (or NULL).
 *
 * @return Index of the terminated process, or a number greater than or equal to
 *         count if all values in the array where NULL.
 */
size_t cmd_wait_any(size_t count, command_t **handles, int *exit_code);

/**
 * Waits for all child processes to terminate.
 */
void cmd_wait_all(void);

#if defined(UNITY_BUILD)
#include "command_builder.c"
#endif
#endif
