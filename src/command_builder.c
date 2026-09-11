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

#if !defined(UNITY_BUILD)
#include "command_builder.h"
#endif

#define TYPE_0 command_builder_t
#define TYPE_1 char *
#define PREFIX cmd_
#define SUFFIX _
#define HEADER_ONLY
#include "dynamic_array.h"

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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

struct command {
  pid_t pid;
  command_builder_t *owner;
};

_Noreturn static void cmd_error(command_builder_t *cmd) {

  if (cmd->handle_error) {
    int errc = errno;
    cmd->handle_error(cmd, errc, sv_cstr(strerror(errc)),
                      cmd->handle_error_arg);
  }
  exit(1);
}

_Noreturn void cmd_rebuild_self(const char *file, const char *cc, ...) {

  env_t env = {
      .allocator = malloc_allocator,
  };
  env_inherit_environ(&env);

  command_builder_t command = {.allocator = malloc_allocator, .env = &env};

  if (!cmd_find_executable(&command, cc)) {
    fprintf(stderr, "Could not find compiler %s\n", cc);
    exit(1);
  }

  va_list va;
  va_start(va, cc);

  // CFLAGS
  for (;;) {
    const char *arg = va_arg(va, const char *);
    if (NULL == arg) {
      break;
    }
    cmd_append(&command, arg);
  }

  // LDFLAGS
  for (;;) {
    const char *arg = va_arg(va, const char *);
    if (NULL == arg) {
      break;
    }
    cmd_append(&command, arg);
  }

  va_end(va);

  env_define(&env, SV("file"), c_file_pattern(sv_cstr(file)));
  cmd_expand_all(&command, "${file}.c", "-o", "${file}", NULL);

  execv(command.items[0], command.items);
  fprintf(stderr, "execv failed\n");
  exit(1);
}

void cmd_write_compilation_database(command_builder_t *cmd) {
  FILE *f = fopen("compile_flags.txt", "w");
  if (NULL == f) {
    return;
  }
  for (size_t i = 0; i < cmd->count; ++i) {
    fputs(cmd->items[i], f);
    fputc('\n', f);
  }
  fclose(f);
}

void cmd_compile(command_builder_t *cmd, const char *src_dir,
                 const char *out_dir, ...) {
  allocator_t *old_scratch = scratch_allocator_pop();
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
  scratch_allocator_restore(old_scratch);
}

void cmd_ensure_directory(const char *dirname) {
  struct stat st;
  if (stat(dirname, &st) == -1) {
    mkdir(dirname, 0777);
  }
}

void cmd_report_errors(command_builder_t *cmd, int ec, string_view_t message,
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

void cmd_enable_error_output(command_builder_t *cmd) {
  cmd->handle_error = cmd_report_errors;
}

void cmd_run_test(command_builder_t *cmd, const char *test_dir,
                  const char *out_dir, const char *test, ...) {
  allocator_t *old_allocator = scratch_allocator_pop();
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

  scratch_allocator_restore(old_allocator);
}

void cmd_append(command_builder_t *cmd, const char *entry) {

  string_builder_t sb = {
      .allocator = cmd->allocator,
  };
  if (!cmd_reserve_(cmd, cmd->count + 2)) {
    cmd_error(cmd);
  }
  if (!sb_append_cstr(&sb, entry)) {
    cmd_error(cmd);
  }
  cmd->items[cmd->count++] = sb.items;
  cmd->items[cmd->count] = NULL;
}

void cmd_append_all(command_builder_t *cmd, ...) {
  va_list va;
  va_start(va, cmd);
  for (;;) {
    const char *param = va_arg(va, const char *);
    if (!param) {
      break;
    }
    cmd_append(cmd, param);
  }
  va_end(va);
}

void cmd_append_sv(command_builder_t *cmd, string_view_t sv) {

  string_builder_t sb = {
      .allocator = cmd->allocator,
  };
  if (!cmd_reserve_(cmd, cmd->count + 2)) {
    cmd_error(cmd);
  }
  if (!sb_append_sv(&sb, sv)) {
    cmd_error(cmd);
  }
  cmd->items[cmd->count++] = sb.items;
  cmd->items[cmd->count] = NULL;
}

void cmd_format(command_builder_t *cmd, const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  cmd_vformat(cmd, fmt, va);
  va_end(va);
}

void cmd_vformat(command_builder_t *cmd, const char *fmt, va_list va) {
  string_builder_t sb = {
      .allocator = cmd->allocator,
  };
  if (!cmd_reserve_(cmd, cmd->count + 2)) {
    cmd_error(cmd);
  }
  if (!sb_vformat(&sb, fmt, va)) {
    cmd_error(cmd);
  }
  cmd->items[cmd->count++] = sb.items;
  cmd->items[cmd->count] = NULL;
}

void cmd_expand(command_builder_t *cmd, const char *pattern) {

  if (!cmd->env) {
    cmd_append(cmd, pattern);
    return;
  }
  string_builder_t sb = {
      .allocator = cmd->allocator,
  };
  if (!cmd_reserve_(cmd, cmd->count + 2)) {
    cmd_error(cmd);
  }
  if (!env_expand(cmd->env, sv_cstr(pattern), ostream_from_sb(&sb))) {
    cmd_error(cmd);
  }
  cmd->items[cmd->count++] = sb.items;
  cmd->items[cmd->count] = NULL;
}

void cmd_expand_all(command_builder_t *cmd, ...) {
  va_list va;
  va_start(va, cmd);
  for (;;) {
    const char *pattern = va_arg(va, const char *);
    if (NULL == pattern) {
      break;
    }
    cmd_expand(cmd, pattern);
  }
  va_end(va);
}

void cmd_reset(command_builder_t *cmd) { cmd->count = 0; }

void cmd_echo_to(command_builder_t *cmd, FILE *f) { cmd->echo_channel = f; }

void cmd_clone(command_builder_t *cmd, const command_builder_t *source) {
  cmd_reset(cmd);

  if (!cmd->env) {
    cmd->env = source->env;
  }

  if (!cmd->allocator) {
    cmd->allocator = source->allocator;
  }

  if (!cmd->echo_channel) {
    cmd->echo_channel = source->echo_channel;
  }

  if (!cmd->handle_error) {
    cmd->handle_error = source->handle_error;
    cmd->handle_error_arg = source->handle_error_arg;
  }

  for (size_t i = 0; i < source->count; ++i) {
    cmd_append(cmd, source->items[i]);
  }
}

bool cmd_find_executable(command_builder_t *cmd, const char *name) {

  struct stat st;
  if (stat(name, &st) != -1 && (st.st_mode & S_IXUSR)) {
    cmd_append(cmd, name);
    return true;
  }

  string_view_t path;
  env_get(cmd->env, SV("PATH"), &path);

  for (;;) {

    string_view_t entry = sv_cut_ch(&path, ':');
    if (entry.count == 0) {
      break;
    }

    string_builder_t sb = {
        .allocator = cmd->allocator,
    };
    sb_append_sv(&sb, entry);
    if (!sv_ends_with_char(entry, '/')) {
      sb_append_char(&sb, '/');
    }
    sb_append_cstr(&sb, name);

    if (stat(sb_cstr(&sb), &st) != -1 && (st.st_mode & S_IXUSR)) {
      if (!cmd_reserve_(cmd, 2)) {
        cmd_error(cmd);
      }
      cmd->items[0] = sb.items;
      cmd->count = cmd->count == 0 ? 1 : cmd->count;
      cmd->items[cmd->count] = NULL;
      return true;
    }
  }
  return false;
}

command_t *cmd_exec_(command_builder_t *cmd, int *out_error) {

  if (cmd->echo_channel) {
    for (size_t i = 0; i < cmd->count; ++i) {
      if (i != 0) {
        fprintf(cmd->echo_channel, " ");
        fprintf(cmd->echo_channel, "'%s'", cmd->items[i]);
      } else {
        fprintf(cmd->echo_channel, "%s", cmd->items[i]);
      }
    }
    fputc('\n', cmd->echo_channel);
  }

  command_t *result =
      allocator_new(cmd->allocator, sizeof(command_t), _Alignof(command_t));
  if (NULL == result) {
    *out_error = errno;
    return NULL;
  }

  int fds[2];
  if (pipe(fds) == -1) {
    allocator_release(cmd->allocator, result, sizeof(command_t),
                      _Alignof(command_t));
    *out_error = errno;
    return NULL;
  }

  pid_t pid = fork();

  if (pid == -1) {

    *out_error = errno;

    close(fds[0]);
    close(fds[1]);

    allocator_release(cmd->allocator, result, sizeof(command_t),
                      _Alignof(command_t));
    return NULL;

  } else if (pid == 0) {

    close(fds[0]);
    fcntl(fds[1], F_SETFD, FD_CLOEXEC);

    execv(cmd->items[0], cmd->items);

    int err = errno;
    write(fds[1], &err, sizeof(int));

    close(fds[1]);
    exit(1);
  }

  close(fds[1]);

  ssize_t nr = read(fds[0], out_error, sizeof(int));
  int err = errno;

  close(fds[0]);

  if (nr == -1) {
    *out_error = err;
    allocator_release(cmd->allocator, result, sizeof(command_t),
                      _Alignof(command_t));
    return NULL;
  } else if (nr != sizeof(int) && nr != 0) {
    *out_error = EBADMSG;
    allocator_release(cmd->allocator, result, sizeof(command_t),
                      _Alignof(command_t));
    return NULL;
  } else if (nr == sizeof(int)) {
    allocator_release(cmd->allocator, result, sizeof(command_t),
                      _Alignof(command_t));
    return NULL;
  }
  result->pid = pid;
  result->owner = cmd;
  return result;
}

int cmd_exec_sync(command_builder_t *cmd) {
  return cmd_wait(cmd_exec_async(cmd));
}

int cmd_exec_sync_maybe(command_builder_t *cmd) {
  int err = 0;
  command_t *result = cmd_exec_(cmd, &err);
  if (result) {
    return cmd_wait(result);
  }
  return err;
}

command_t *cmd_exec_async_maybe(command_builder_t *cmd) {
  int err;
  return cmd_exec_(cmd, &err);
}

command_t *cmd_exec_async(command_builder_t *cmd) {
  int err = 0;
  command_t *result = cmd_exec_(cmd, &err);

  if (NULL == result) {
    errno = err;
    cmd_error(cmd);
  }
  return result;
}

int cmd_wait(command_t *handle) {

  if (NULL == handle) {
    return -1;
  }

  int stat;
  pid_t result = waitpid(handle->pid, &stat, 0);

  allocator_release(handle->owner->allocator, handle, sizeof(command_t),
                    _Alignof(command_t));

  if (result == -1) {
    cmd_error(handle->owner);
  }
  return WIFEXITED(stat) ? WEXITSTATUS(stat) : -1;
}

bool cmd_poll(command_t *handle, int *exit_code) {

  if (NULL == handle) {
    if (exit_code) {
      *exit_code = -1;
    }
    return true;
  }

  int stat;
  pid_t result = waitpid(handle->pid, &stat, WNOHANG);

  if (result == -1) {
    cmd_error(handle->owner);
  } else if (result == 0) {
    return false;
  }

  if (exit_code) {
    *exit_code = WIFEXITED(stat) ? WEXITSTATUS(stat) : -1;
  }

  allocator_release(handle->owner->allocator, handle, sizeof(command_t),
                    _Alignof(command_t));
  return true;
}

void cmd_wait_all(void) { waitpid(-1, NULL, 0); }

void cmd_wait_every(size_t count, command_t **handles, int *exit_codes) {
  for (size_t i = 0; i < count; ++i) {
    int ec = cmd_wait(handles[i]);
    if (exit_codes) {
      exit_codes[i] = ec;
    }
  }
  memset(handles, 0, count * sizeof(command_t *));
}

size_t cmd_wait_any(size_t count, command_t **handles, int *exit_code) {

  for (size_t i = 0; i < count; ++i) {

    if (!handles[i]) {
      continue;
    }

    if (cmd_poll(handles[i], exit_code)) {
      handles[i] = NULL;
      return i;
    }
  }

  for (size_t i = 0; i < count; ++i) {

    if (!handles[i]) {
      continue;
    }

    int ec = cmd_wait(handles[i]);

    if (exit_code) {
      *exit_code = ec;
    }

    handles[i] = NULL;
    return i;
  }
  return count;
}
