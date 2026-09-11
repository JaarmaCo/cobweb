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
 * @file command_line.h
 *
 * Defines utility functions for parsing the command line.
 */

#if !defined(COMMAND_LINE_H_)
#define COMMAND_LINE_H_

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "string_view.h"

/**
 * Shifts the command line one argument to the left.
 */
static inline void cl_shift(int *argc, char ***argv) {
  assert(argc && argv);
  assert(*argc > 0);

  --*argc;
  ++*argv;
}

/**
 * Parses a command line switch. If the switch was matched, the command line is
 * shifted, and true is returned.
 *
 * @param flag Name of the command line switch.
 * @param[inout] argc Pointer to argc
 * @param[inout] argv Pointer to argv
 */
static inline bool cl_switch(const char *flag, int *argc, char ***argv) {
  if (strcmp(flag, **argv) == 0) {
    cl_shift(argc, argv);
    return true;
  }
  return false;
}

/**
 * Parses a command line argument. If the flag is matched, the command line is
 * shifted twice, and the value of the argument is written out.
 *
 * If this function parses an incomplete argument, an error is printed to
 * stderr, and exit(int) is called.
 *
 * @param flag Command line argument to parse.
 * @param[out] out_value Pointer to a variable that receives the parsed value.
 * @param[inout] argc Pointer to argc
 * @param[inout] argv Pointer to argv
 */
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

#endif
