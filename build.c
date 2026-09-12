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
 * @file build.c
 *
 * The following file contains both the source code used to build the C source
 * files in this project. It also serves as an example for how the command
 * builder utility may be used to compile other C projects.
 *
 * Credit to nob.h (https://github.com/tsoding/nob.h) for being the inspiration
 * behind this build utility.
 */

// The UNITY_BUILD option will #include every .c file dependency into this
// translation unit.
#define UNITY_BUILD
#include "command_builder.h"
#include "command_line.h"

#define OUT_DIR "out/"
#define SRC_DIR "src/"
#define TEST_DIR "test/"

// clang-format off

// CC : Defines the compiler to use, is automatically picked up by
//      CMD_REBUILD_SELF later.
#define CC "gcc"

// LDFLAGS, CFLAGS : GNU Makefile inspired macro definitions. These
//                   are also automatically used by CMD_REBUILD_SELF.
#define LDFLAGS \
  "-lm",        \
  NULL
#define CFLAGS  \
  "-Wall",      \
  "-Wextra",    \
  "-Wpedantic", \
  "-Werror",    \
  "-ggdb",      \
  "-Isrc",      \
  NULL

#define SOURCES \
  "allocator.c", \
  "arena_allocator.c", \
  "auto_free.c", \
  "command_builder.c", \
  "env.c", \
  "format.c", \
  "json.c", \
  "json_parse.c", \
  "json_value_parser.c", \
  "scratch_allocator.c", \
  "stream.c", \
  "string_builder.c", \
  "string_view.c", \
  "sv_read.c", \
  "concurrency/thread.c", \
  NULL

#define TESTS \
  "dynamic_array.c", \
  "hashmap.c", \
  "json.c", \
  "string_builder.c", \
  "thread.c", \
  NULL

// clang-format on

int main(int argc, char **argv) {

  // Example usage of the cl_* command line parser utilities.
  //
  bool run_tests = false;

  while (argc > 0) {
    if (cl_switch("--self", &argc, &argv)) {
      CMD_REBUILD_SELF();
    } else if (cl_switch("--test", &argc, &argv)) {
      run_tests = true;
    } else {
      cl_shift(&argc, &argv);
    }
  }

  // Linking this env_t instance to command_builder_t forwards the current shell
  // environment for later use by the cmd_* commands.
  //
  env_t env = {
      .allocator = malloc_allocator,
  };
  env_inherit_environ(&env);

  command_builder_t cc = {
      .allocator = malloc_allocator,
      .env = &env,
  };
  cmd_echo_to(&cc, stdout);
  cmd_enable_error_output(&cc);

  if (!cmd_find_executable(&cc, CC)) {
    fputs(CC " is not a known executable.", stderr);
    exit(1);
  }
  cmd_append_all(&cc, CFLAGS);

  // #if guard lets the build file produce a compilation database at the first
  // run (I.e the "bootstrap" build).
#if defined(BOOTSTRAP_BUILD)

  cmd_write_compilation_database(&cc);
  CMD_REBUILD_SELF();

#else

  cmd_ensure_directory(OUT_DIR);
  cmd_ensure_directory(OUT_DIR "concurrency/");
  cmd_ensure_directory(OUT_DIR TEST_DIR);

  cmd_compile(&cc, SRC_DIR, OUT_DIR, SOURCES);
  cmd_compile(&cc, TEST_DIR, OUT_DIR TEST_DIR, TESTS);

  if (run_tests) {

    cmd_append_all(&cc, LDFLAGS);

    const char *tests[] = {TESTS};
    for (size_t i = 0; tests[i]; ++i) {
      cmd_run_test(&cc, TEST_DIR, OUT_DIR, tests[i], SOURCES);
    }
  }
  return 0;
#endif
}
