#define UNITY_BUILD
#include "command_builder.h"
#include "command_line.h"

#include <sys/stat.h>

#define TYPE_1 command_t *
#define HEADER_ONLY
#define PREFIX da_
#define SUFFIX _cmd
#include "dynamic_array.h"

#define TYPE_1 int
#define HEADER_ONLY
#define PREFIX da_
#define SUFFIX _i
#include "dynamic_array.h"

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

void compile(command_builder_t *cmd, const char *src_dir, const char *out_dir,
             ...) {

  allocator_t *scratch = scratch_allocator(1024 * 1024);
  command_builder_t local = {
      .allocator = scratch,
  };
  dynamic_array_cmd runs = {
      .allocator = scratch,
  };
  dynamic_array_i exit_codes = {
      .allocator = scratch,
  };
  va_list va;
  va_start(va, out_dir);
  for (;;) {

    const char *src = va_arg(va, const char *);
    if (NULL == src) {
      break;
    }

    cmd_clone(&local, cmd);

    string_view_t filename = sv_cstr(src);
    filename = c_file_pattern(filename);

    env_define(local.env, SV("file"), filename);
    env_define(local.env, SV("src_dir"), sv_cstr(src_dir));
    env_define(local.env, SV("out_dir"), sv_cstr(out_dir));

    cmd_expand_all(&local, "-c", "${src_dir}${file}.c", "-o",
                   "${out_dir}${file}.o", NULL);

    da_append_cmd(&runs, cmd_exec_async(&local));
    da_append_i(&exit_codes, 0);
  }
  va_end(va);

  cmd_wait_every(runs.count, runs.items, exit_codes.items);
  for (size_t i = 0; i < exit_codes.count; ++i) {
    if (exit_codes.items[i] != 0) {
      fprintf(stderr, "%s exited with a nonzero exit code", cmd->items[0]);
      exit(1);
    }
  }
}

static void report_error(command_builder_t *cmd, int ec, string_view_t message,
                         void *user) {
  (void)ec;
  (void)user;
  fprintf(stderr, "ERROR: ");
  for (size_t i = 0; i < cmd->count; ++i) {
    if (i != 0) {
      fputc(' ', stderr);
    }
    fprintf(stderr, "%s", cmd->items[i]);
  }
  fprintf(stderr, " :: %.*s", (int)message.count, message.items);
}

void run_test(command_builder_t *cmd, const char *test_dir, const char *out_dir,
              const char *test, ...) {
  allocator_t *allocator = scratch_allocator(1024 * 1024);
  command_builder_t cc = {
      .allocator = allocator,
  };
  cmd_clone(&cc, cmd);

  env_define(cc.env, SV("out_dir"), sv_cstr(out_dir));
  env_define(cc.env, SV("test_dir"), sv_cstr(test_dir));

  va_list va;
  va_start(va, test);
  for (;;) {

    const char *arg = va_arg(va, const char *);
    if (!arg) {
      break;
    }

    env_define(cc.env, SV("file"), c_file_pattern(sv_cstr(arg)));
    cmd_expand(&cc, "${out_dir}${file}.o");
  }
  va_end(va);

  env_define(cc.env, SV("file"), c_file_pattern(sv_cstr(test)));
  cmd_expand_all(&cc, "-o", "${out_dir}${test_dir}${file}",
                 "${test_dir}${file}.c", NULL);
  int ec = cmd_exec_sync(&cc);
  if (ec != 0) {
    fprintf(stderr, "%s exited with a nonzero exit code.\n", cc.items[0]);
    exit(1);
  }

  cmd_reset(&cc);
  cmd_expand(&cc, "./${out_dir}${test_dir}${file}");

  ec = cmd_exec_sync(&cc);
  if (ec != 0) {
    fprintf(stderr, "Test %s failed with exit code %d\n", test, ec);
    exit(1);
  }
}

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
      .handle_error = report_error,
  };
  cmd_echo_to(&cc, stdout);

  struct stat st;
  if (stat("./" OUT_DIR, &st) == -1) {
    puts("mkdir ./" OUT_DIR);
    mkdir("./" OUT_DIR, 0777);
  }

  if (stat("./" OUT_DIR TEST_DIR, &st) == -1) {
    puts("mkdir ./" OUT_DIR TEST_DIR);
    mkdir("./" OUT_DIR TEST_DIR, 0777);
  }

  if (!cmd_find_executable(&cc, CC)) {
    fputs(CC " is not a known executable.", stderr);
    exit(1);
  }
  cmd_append_all(&cc, CFLAGS);

  compile(&cc, SRC_DIR, OUT_DIR, SOURCES);
  compile(&cc, TEST_DIR, OUT_DIR TEST_DIR, TESTS);

  cmd_append_all(&cc, LDFLAGS);

  const char *tests[] = {TESTS};
  for (size_t i = 0; tests[i]; ++i) {
    run_test(&cc, TEST_DIR, OUT_DIR, tests[i], SOURCES);
  }
  return 0;
}
