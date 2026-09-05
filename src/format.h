#ifndef FORMAT_H_

#include <stdarg.h>

#include "string_view.h"

#define FORMAT_INPUT_VALUE -2

typedef struct format_specifier {
  int modifier;
  int field_width;
  int precision;
  char length[4];
  int specifier;
} format_specifier_t;

int fmt_parse_specifier(string_view_t *fmt, format_specifier_t *directive);

void fmt_fetch_lengths(format_specifier_t *directive, va_list *va);

#define FORMAT_H_
#endif
