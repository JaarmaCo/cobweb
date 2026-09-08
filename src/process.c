#include "process.h"
#include "string_builder.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <spawn.h>
#include <unistd.h>
#include <wait.h>

extern char **environ;

int devnull = -1;

int open_devnull(void) {
  if (devnull != -1) {
    return devnull;
  }
  devnull = open("/dev/null", O_WRONLY);
  if (devnull == -1) {
    fprintf(stderr, "/dev/null could not be opened");
    exit(1);
  }
  return devnull;
}

struct process {
  pid_t pid;
  string_builder_t collected_stdout;
  string_builder_t collected_stderr;
  int stat_loc;
  FILE *open_files[8];
};

struct stream_args {
  int (*get)(void *);
  void *in_handle;
  int (*put)(int, void *);
  void *out_handle;
};

static void *do_stream(void *argp) {
  struct stream_args args = *(struct stream_args *)argp;
  free(argp);
  for (;;) {
    int ch = args.get(args.in_handle);
    if (ch == -1) {
      break;
    }
    if (args.put(ch, args.out_handle) == -1) {
      break;
    }
  }
  return NULL;
}

static bool default_stream_callback(int (*get)(void *), void *in_handle,
                                    int (*put)(int, void *), void *out_handle,
                                    void *user) {
  (void)user;
  struct stream_args *args = malloc(sizeof(struct stream_args));
  if (NULL == args) {
    return false;
  }
  args->get = get;
  args->in_handle = in_handle;
  args->put = put;
  args->out_handle = out_handle;
  pthread_t thread;
  if (pthread_create(&thread, NULL, do_stream, args)) {
    return false;
  }
  pthread_detach(thread);
  return true;
}

static void add_file(process_t *p, FILE *f) {
  for (size_t i = 0; i < 8; ++i) {
    if (!p->open_files[i]) {
      p->open_files[i] = f;
      return;
    }
  }
}

static int put_sb(int ch, void *sb) {
  return sb_append_char(sb, (char)ch) ? 0 : -1;
}

static bool
handle_file_action(process_t *process, const process_file_t *file, int std,
                   int pipefds[2], posix_spawn_file_actions_t *actions,
                   bool (*stream)(int (*)(void *), void *, int (*)(int, void *),
                                  void *, void *),
                   void *user) {

  switch (file->usage) {
  case PROCESS_FILE_SET: {
    // Set to the literal file
    if (!file->handle) {
      perror("File handle was NULL");
      errno = EBADF;
      return false;
    }
    int fd = fileno(file->handle);
    if (fd != -1) {
      // FILE pointed to a system file, just dup it.
      if (posix_spawn_file_actions_adddup2(actions, fd, std)) {
        return false;
      }
      return true;
    }

    // FILE was not a system file, use a stream transfer via pipe
    if (std == STDIN_FILENO) {

      if (pipe(pipefds)) {
        return false;
      }

      FILE *f = fdopen(pipefds[1], "w");
      if (NULL == f) {
        return false;
      }
      add_file(process, f);

      if (posix_spawn_file_actions_addclose(actions, pipefds[0])) {
        return false;
      }

      if (posix_spawn_file_actions_addclosefrom_np(actions, pipefds[1])) {
        return false;
      }

      if (posix_spawn_file_actions_adddup2(actions, pipefds[0], STDIN_FILENO)) {
        return false;
      }

      return stream((int (*)(void *))fgetc, file->handle,
                    (int (*)(int, void *))fputc, f, user);

    } else {

      if (pipe(pipefds)) {
        return false;
      }

      FILE *f = fdopen(pipefds[0], "r");
      if (NULL == f) {
        return false;
      }
      add_file(process, f);

      if (posix_spawn_file_actions_addclose(actions, pipefds[1])) {
        return false;
      }

      if (posix_spawn_file_actions_addclosefrom_np(actions, pipefds[0])) {
        return false;
      }

      if (posix_spawn_file_actions_adddup2(actions, pipefds[1], std)) {
        return false;
      }
      return stream((int (*)(void *))fgetc, f, (int (*)(int, void *))fputc,
                    file->handle, user);
    }
  } break;
  case PROCESS_FILE_DEVNULL:
    // Direct file to /dev/null
    if (posix_spawn_file_actions_adddup2(actions, open_devnull(), std)) {
      return false;
    }
    break;
  case PROCESS_FILE_INHERIT:
    /* Inherited by default */
    break;
  case PROCESS_FILE_PIPE: {
    // Pipe file to a process buffer.

    if (std == STDIN_FILENO) {
      perror("Invalid action argument for stdin");
      errno = EBADR;
      return false;
    }

    if (pipe(pipefds)) {
      return false;
    }

    FILE *f = fdopen(pipefds[0], "r");
    if (NULL == f) {
      return false;
    }

    add_file(process, f);

    if (posix_spawn_file_actions_addclose(actions, pipefds[1])) {
      return false;
    }

    if (posix_spawn_file_actions_addclosefrom_np(actions, pipefds[0])) {
      return false;
    }

    if (posix_spawn_file_actions_adddup2(actions, pipefds[1], std)) {
      return false;
    }

    return stream((int (*)(void *))fgetc, f, put_sb,
                  std == STDOUT_FILENO ? &process->collected_stdout
                                       : &process->collected_stderr,
                  user);

    break;
  }
  }

  return true;
}

static bool build_argument_list(allocator_t *allocator,
                                string_builder_t *executable_path,
                                char ***strings, size_t **string_capacities,
                                const process_arguments_t *arguments) {

  if (!sb_append_sv(executable_path, arguments->executable)) {
    return false;
  }

  *strings =
      allocator_new(allocator, (arguments->argument_count + 1) * sizeof(char *),
                    _Alignof(char *));
  if (NULL == strings) {
    return false;
  }
  *string_capacities = allocator_new(
      allocator, arguments->argument_count * sizeof(size_t), _Alignof(size_t));
  if (NULL == string_capacities) {
    return false;
  }

  for (size_t i = 0; i < arguments->argument_count; ++i) {
    string_builder_t sb = {.allocator = allocator};
    if (!sb_append_sv(&sb, arguments->arguments[i])) {
      return false;
    }
    strings[0][i] = sb.items;
    string_capacities[0][i] = sb.capacity;
  }
  strings[arguments->argument_count] = NULL;
  return true;
}

process_t *process_launch(const process_arguments_t *arguments) {

  process_t *result = NULL;
  char **strings = NULL;
  size_t *string_capacities = NULL;
  allocator_t *allocator =
      arguments->allocator ? arguments->allocator : malloc_allocator;
  posix_spawn_file_actions_t actions = {0};
  posix_spawnattr_t attrs = {0};
  int in_pipe[2] = {-1, -1}, out_pipe[2] = {-1, -1}, err_pipe[2] = {-1, -1};
  process_t *process = NULL;

  bool (*stream_callback)(int (*)(void *), void *, int (*)(int, void *), void *,
                          void *);
  stream_callback = arguments->stream_callback ? arguments->stream_callback
                                               : default_stream_callback;

  string_builder_t executable_path;
  executable_path = (string_builder_t){
      .allocator = allocator,
  };

  if (!build_argument_list(allocator, &executable_path, &strings,
                           &string_capacities, arguments)) {
    goto defer;
  }

  process = allocator_new(allocator, sizeof(process_t), _Alignof(process_t));
  if (NULL == process) {
    goto defer;
  }

  memset(process, 0, sizeof *process);
  process->collected_stderr.allocator = allocator;
  process->collected_stdout.allocator = allocator;

  if (posix_spawnattr_init(&attrs)) {
    goto defer;
  }

  if (posix_spawn_file_actions_init(&actions)) {
    goto defer;
  }

  if (!handle_file_action(process, &arguments->in, STDIN_FILENO, in_pipe,
                          &actions, stream_callback, arguments->user)) {
    goto defer;
  }

  if (!handle_file_action(process, &arguments->out, STDOUT_FILENO, out_pipe,
                          &actions, stream_callback, arguments->user)) {
    goto defer;
  }

  if (!handle_file_action(process, &arguments->err, STDERR_FILENO, err_pipe,
                          &actions, stream_callback, arguments->user)) {
    goto defer;
  }

  pid_t pid = 0;
  int ec = posix_spawn(&pid, sb_cstr(&executable_path), &actions, &attrs,
                       strings, environ);

  if (ec != 0) {
    goto defer;
  }
  result = process;
  process = NULL;

  result->pid = pid;

defer:

  // Free half-completed process handle
  if (process) {
    for (size_t i = 0; i < 8; ++i) {
      if (process->open_files[i]) {
        fclose(process->open_files[i]);
      } else {
        break;
      }
    }

    allocator_release(allocator, process, sizeof(process_t),
                      _Alignof(process_t));
  }

  // Free attributes
  posix_spawnattr_destroy(&attrs);
  posix_spawn_file_actions_destroy(&actions);

  // Free argument list
  if (NULL != strings && NULL != string_capacities) {
    for (size_t i = 0; i < arguments->argument_count; ++i) {
      allocator_release(allocator, strings[i], string_capacities[i],
                        _Alignof(char));
    }
  }
  sb_destroy(&executable_path);
  allocator_release(allocator, strings,
                    (arguments->argument_count + 1) * sizeof(char *),
                    _Alignof(char *));
  allocator_release(allocator, string_capacities,
                    arguments->argument_count * sizeof(char *),
                    _Alignof(char *));

  // Close pipes on failure
  if (!result) {
    if (in_pipe[0] != -1)
      close(in_pipe[0]);
    if (in_pipe[1] != -1)
      close(in_pipe[1]);

    if (out_pipe[0] != -1)
      close(out_pipe[0]);
    if (out_pipe[1] != -1)
      close(out_pipe[1]);

    if (err_pipe[0] != -1)
      close(err_pipe[0]);
    if (err_pipe[1] != -1)
      close(err_pipe[1]);
  }
  return result;
}

bool process_wait(process_t *process, process_wait_result_t **out_result) {

  pid_t result = waitpid(process->pid, &process->stat_loc, 0);
  if (result == -1) {
    return false;
  }

  *out_result = (process_wait_result_t *)process;
  return true;
}

int process_get_exit_code(const process_wait_result_t *result) {
  const process_t *process = (const process_t *)result;
  return WEXITSTATUS(process->stat_loc);
}
