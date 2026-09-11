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
#include "format.h"
#endif

void fmt_fetch_lengths(format_specifier_t *spec, va_list *va) {
  if (spec->field_width == FORMAT_INPUT_VALUE) {
    spec->field_width = va_arg(*va, int);
  }
  if (spec->precision == FORMAT_INPUT_VALUE) {
    spec->precision = va_arg(*va, int);
  }
}

int fmt_parse_specifier(string_view_t *out_fmt, format_specifier_t *directive) {
  string_view_t fmt = *out_fmt;
  if (fmt.count < 2 || fmt.items[0] != '%') {
    return -1;
  }

  directive->field_width = -1;
  directive->length[0] = 0;
  directive->length[1] = 0;
  directive->length[2] = 0;
  directive->length[3] = 0;
  directive->precision = -1;
  directive->modifier = -1;
  directive->specifier = -1;

  fmt = sv_drop(fmt, 1);
  if (*fmt.items == '%') {
    fmt = sv_drop(fmt, 1);
    directive->specifier = '%';
    *out_fmt = fmt;
    return 0;
  }

  if (fmt.count == 0) {
    return -1;
  }

  switch (*fmt.items) {
  case '-':
  case '+':
  case '#':
  case '0':
    directive->modifier = *fmt.items;
    fmt = sv_drop(fmt, 1);
    break;
  }

  switch (*fmt.items) {
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': {
    intmax_t d;
    fmt = sv_read_imax(fmt, &d, 10);
    directive->field_width = (int)d;
  } break;
  case '*':
    directive->field_width = FORMAT_INPUT_VALUE;
    fmt = sv_drop(fmt, 1);
    break;
  }

  if (fmt.count == 0) {
    return -1;
  }

  if (*fmt.items == '.') {
    fmt = sv_drop(fmt, 1);

    if (fmt.count == 0) {
      return -1;
    }

    switch (*fmt.items) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      intmax_t d;
      fmt = sv_read_imax(fmt, &d, 10);
      directive->field_width = (int)d;
    } break;
    case '*':
      directive->field_width = FORMAT_INPUT_VALUE;
      fmt = sv_drop(fmt, 1);
      break;
    }

    if (fmt.count == 0) {
      return -1;
    }
  }

  switch (*fmt.items) {
  case 'l':
    if (fmt.count > 1 && fmt.items[1] == 'l') {
      directive->length[0] = 'l';
      directive->length[1] = 'l';
      fmt = sv_drop(fmt, 2);
    } else {
      directive->length[0] = 'l';
      fmt = sv_drop(fmt, 1);
    }
    break;
  case 'h':
    if (fmt.count > 1 && fmt.items[1] == 'h') {
      directive->length[0] = 'h';
      directive->length[1] = 'h';
      fmt = sv_drop(fmt, 2);
    } else {
      directive->length[0] = 'h';
      fmt = sv_drop(fmt, 1);
    }
    break;
  case 'j':
  case 'z':
  case 't':
  case 'L':
    directive->length[0] = *fmt.items;
    fmt = sv_drop(fmt, 1);
    break;
  }

  if (fmt.count == 0) {
    return -1;
  }

  directive->specifier = *fmt.items;
  *out_fmt = sv_drop(fmt, 1);
  return 0;
}
