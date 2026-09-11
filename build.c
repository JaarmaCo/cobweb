#define UNITY_BUILD
#include "command_builder.h"
#include "command_line.h"

#include <sys/stat.h>

#define CC "gcc"
#define OUT_DIR "out/"
#define SRC_DIR "src/"
#define TEST_DIR "test/"
#define LDFLAGS "-lm", NULL

// clang-format off
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
  "process.c", \
  "scratch_allocator.c", \
  "stream.c", \
  "string_builder.c", \
  "string_view.c", \
  "sv_read.c", \
  NULL

#define TESTS \
  "dynamic_array.c", \
  "hashmap.c", \
  "json.c", \
  "string_builder.c", \
  NULL

#define TEST_TARGETS \
  "dynamic_array", \
  "hashmap", \
  "json", \
  "string_builder", \
  NULL

// clang-format on

int main(int argc, char **argv) {

  while (argc > 0) {
    if (cl_switch("--self", &argc, &argv)) {
      CMD_REBUILD_SELF();
    }
    cl_shift(&argc, &argv);
  }

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

  cmd_ensure_directory(OUT_DIR);
  cmd_ensure_directory(OUT_DIR TEST_DIR);

  if (!cmd_find_executable(&cc, CC)) {
    fputs(CC " is not a known executable.", stderr);
    exit(1);
  }
  cmd_append_all(&cc, CFLAGS);

  cmd_compile(&cc, SRC_DIR, OUT_DIR, SOURCES);
  cmd_compile(&cc, TEST_DIR, OUT_DIR TEST_DIR, TESTS);

  cmd_append_all(&cc, LDFLAGS);

  const char *tests[] = {TESTS};
  for (size_t i = 0; tests[i]; ++i) {
    cmd_run_test(&cc, TEST_DIR, OUT_DIR, tests[i], SOURCES);
  }
  return 0;
}
