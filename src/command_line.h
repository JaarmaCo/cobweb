#if !defined(COMMAND_LINE_H_)

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "string_view.h"

static inline void cl_shift(int *argc, char ***argv) {
  assert(argc && argv);
  assert(*argc > 0);

  --*argc;
  ++*argv;
}

static inline bool cl_switch(const char *flag, int *argc, char ***argv) {
  if (strcmp(flag, **argv) == 0) {
    cl_shift(argc, argv);
    return true;
  }
  return false;
}

static inline bool cl_arg(const char *flag, string_view_t *out_value, int *argc,
                          char ***argv) {
  if (strcmp(flag, **argv) == 0) {
    cl_shift(argc, argv);

    if (*argc == 0) {
      fprintf(stderr, "%s requires an argument.\n", flag);
      exit(2);
    }

    *out_value = sv_cstr(**argv);
    cl_shift(argc, argv);
  }
  return true;
}

#define COMMAND_LINE_H_
#endif
