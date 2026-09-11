
#if !defined(UNITY_BUILD)
#include "command_builder.h"
#endif

#define TYPE_0 command_builder_t
#define TYPE_1 char *
#define PREFIX cmd_
#define SUFFIX _
#define HEADER_ONLY
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

static char *cmd_find_executable_(command_builder_t *cmd,
                                  string_builder_t *out_str) {
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
    sb_append_cstr(&sb, cmd->items[0]);

    struct stat st;
    if (stat(sb_cstr(&sb), &st) != -1 && (st.st_mode & S_IXUSR)) {
      *out_str = sb;
      return sb_cstr(&sb);
    }
    sb_destroy(&sb);
  }
  return NULL;
}

command_t *cmd_exec_(command_builder_t *cmd, int *out_error) {

  string_builder_t sb = {0};
  char *executable = cmd_find_executable_(cmd, &sb);
  if (!executable) {
    *out_error = ENOENT;
    return NULL;
  }

  if (cmd->echo_channel) {
    for (size_t i = 0; i < cmd->count; ++i) {
      if (i != 0) {
        fprintf(cmd->echo_channel, " ");
        fprintf(cmd->echo_channel, "'%s'", cmd->items[i]);
      } else {
        fprintf(cmd->echo_channel, "%s", executable);
      }
    }
    fputc('\n', cmd->echo_channel);
  }

  command_t *result =
      allocator_new(cmd->allocator, sizeof(command_t), _Alignof(command_t));
  if (NULL == result) {
    sb_destroy(&sb);
    *out_error = errno;
    return NULL;
  }

  int fds[2];
  if (pipe(fds) == -1) {
    sb_destroy(&sb);
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

    sb_destroy(&sb);
    allocator_release(cmd->allocator, result, sizeof(command_t),
                      _Alignof(command_t));
    return NULL;

  } else if (pid == 0) {

    close(fds[0]);
    fcntl(fds[1], FD_CLOEXEC);

    execv(executable, cmd->items);

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
    sb_destroy(&sb);
    allocator_release(cmd->allocator, result, sizeof(command_t),
                      _Alignof(command_t));
    return NULL;
  } else if (nr != sizeof(int) && nr != 0) {
    *out_error = EBADMSG;
    sb_destroy(&sb);
    allocator_release(cmd->allocator, result, sizeof(command_t),
                      _Alignof(command_t));
    return NULL;
  } else if (nr == sizeof(int)) {
    sb_destroy(&sb);
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
