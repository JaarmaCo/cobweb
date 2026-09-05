#include "format.h"
#include "string_view.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static int digit_of_base(char ch, int base) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0' < base ? ch - '0' : -1;
  } else if (ch >= 'a' && ch <= 'f') {
    return (ch - 'a') + 10 < base ? ch - 'a' + 10 : -1;
  } else if (ch >= 'A' && ch <= 'F') {
    return (ch - 'A') + 10 < base ? ch - 'A' + 10 : -1;
  }
  return -1;
}

static string_view_t parse_sized_integer(string_view_t sv, uintmax_t *out,
                                         int base, size_t type_size) {
  if (base == 0) {
    if (sv_starts_with_substr(sv, sv_cstr("0x")) ||
        sv_starts_with_substr(sv, sv_cstr("0X"))) {
      base = 16;
    } else {
      base = 10;
    }
  }

  if (base > 16 || base < 2) {
    return sv;
  }

  sv = sv_trim_left(sv, "0");

  const int MAX_DIGITS[4][16] = {
      {8, 6, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2},
      {16, 11, 8, 7, 7, 6, 6, 6, 5, 5, 5, 5, 5, 5, 4},
      {32, 21, 16, 14, 13, 12, 11, 11, 10, 10, 9, 9, 9, 9, 8},
      {64, 41, 32, 28, 25, 23, 22, 21, 20, 19, 18, 18, 17, 17, 16}};
  int digit_count = 0;
  uintmax_t result = 0;
  while (sv.count > 0 && digit_count <= MAX_DIGITS[type_size - 1][base - 2]) {
    int d = digit_of_base(*sv.items, base);
    if (d == -1) {
      break;
    }
    sv = sv_drop(sv, 1);
    result *= (uintmax_t)base;
    result += (uintmax_t)d;
    ++digit_count;
  }
  *out = result;
  return sv;
}

static string_view_t parse_signed_sized_integer(string_view_t sv, intmax_t *out,
                                                int base, size_t type_size) {
  bool sign = false;
  if (sv_starts_with_char(sv, '-')) {
    sv = sv_drop(sv, 1);
    sign = true;
  }
  uintmax_t uz;
  sv = parse_sized_integer(sv, &uz, base, type_size);
  *out = sign ? -(intmax_t)uz : (intmax_t)uz;
  return sv;
}

string_view_t sv_read_u8(string_view_t sv, uint8_t *out, int base) {
  uintmax_t parsed;
  sv = parse_sized_integer(sv, &parsed, base, 1);
  *out = (uint8_t)parsed;
  return sv;
}

string_view_t sv_read_u16(string_view_t sv, uint16_t *out, int base) {
  uintmax_t parsed;
  sv = parse_sized_integer(sv, &parsed, base, 2);
  *out = (uint16_t)parsed;
  return sv;
}

string_view_t sv_read_u32(string_view_t sv, uint32_t *out, int base) {
  uintmax_t parsed;
  sv = parse_sized_integer(sv, &parsed, base, 3);
  *out = (uint32_t)parsed;
  return sv;
}

string_view_t sv_read_u64(string_view_t sv, uint64_t *out, int base) {
  uintmax_t parsed;
  sv = parse_sized_integer(sv, &parsed, base, 4);
  *out = (uint64_t)parsed;
  return sv;
}

string_view_t sv_read_i8(string_view_t sv, int8_t *out, int base) {
  intmax_t parsed;
  sv = parse_signed_sized_integer(sv, &parsed, base, 1);
  *out = (int8_t)parsed;
  return sv;
}

string_view_t sv_read_i16(string_view_t sv, int16_t *out, int base) {
  intmax_t parsed;
  sv = parse_signed_sized_integer(sv, &parsed, base, 2);
  *out = (int16_t)parsed;
  return sv;
}

string_view_t sv_read_i32(string_view_t sv, int32_t *out, int base) {
  intmax_t parsed;
  sv = parse_signed_sized_integer(sv, &parsed, base, 3);
  *out = (int32_t)parsed;
  return sv;
}

string_view_t sv_read_i64(string_view_t sv, int64_t *out, int base) {
  intmax_t parsed;
  sv = parse_signed_sized_integer(sv, &parsed, base, 4);
  *out = (int64_t)parsed;
  return sv;
}

string_view_t sv_read_imax(string_view_t sv, intmax_t *out, int base) {
  return parse_signed_sized_integer(sv, out, base, 4);
}

string_view_t sv_read_umax(string_view_t sv, uintmax_t *out, int base) {
  return parse_sized_integer(sv, out, base, 4);
}

string_view_t sv_read_uz(string_view_t sv, size_t *out, int base) {
  return _Generic((*out), uint32_t: sv_read_u32, uint64_t: sv_read_u64)(sv, out,
                                                                        base);
}

string_view_t sv_read_iz(string_view_t sv, ptrdiff_t *out, int base) {
  return _Generic((*out), int32_t: sv_read_i32, int64_t: sv_read_i64)(sv, out,
                                                                      base);
}

string_view_t sv_read_iptr(string_view_t sv, intptr_t *out, int base) {
  return _Generic((*out), int32_t: sv_read_i32, int64_t: sv_read_i64)(sv, out,
                                                                      base);
}

string_view_t sv_read_uptr(string_view_t sv, uintptr_t *out, int base) {
  return _Generic((*out), uint32_t: sv_read_u32, uint64_t: sv_read_u64)(sv, out,
                                                                        base);
}

string_view_t sv_read_i(string_view_t sv, int *out, int base) {
  return _Generic((*out),
      int16_t: sv_read_i16,
      int32_t: sv_read_i32,
      int64_t: sv_read_i64)(sv, out, base);
}

string_view_t sv_read_l(string_view_t sv, long *out, int base) {
  return _Generic((*out), int32_t: sv_read_i32, int64_t: sv_read_i64)(sv, out,
                                                                      base);
}

string_view_t sv_read_ll(string_view_t sv, long long *out, int base) {
  intmax_t parsed;
  sv = parse_signed_sized_integer(sv, &parsed, base, 4);
  *out = (long long)parsed;
  return sv;
}

string_view_t sv_read_u(string_view_t sv, unsigned int *out, int base) {
  return _Generic((*out),
      uint16_t: sv_read_u16,
      uint32_t: sv_read_u32,
      uint64_t: sv_read_u64)(sv, out, base);
}

string_view_t sv_read_ul(string_view_t sv, unsigned long *out, int base) {
  return _Generic((*out), uint32_t: sv_read_u32, uint64_t: sv_read_u64)(sv, out,
                                                                        base);
}

string_view_t sv_read_ull(string_view_t sv, unsigned long long *out, int base) {
  uintmax_t parsed;
  sv = parse_sized_integer(sv, &parsed, base, 4);
  *out = (unsigned long long)parsed;
  return sv;
}

string_view_t sv_read_f(string_view_t sv, float *out, int base) {
  long double value;
  sv = sv_read_ld(sv, &value, base);
  *out = (float)value;
  return sv;
}

string_view_t sv_read_d(string_view_t sv, double *out, int base) {
  long double value;
  sv = sv_read_ld(sv, &value, base);
  *out = (double)value;
  return sv;
}

string_view_t sv_read_ld(string_view_t sv, long double *out, int base) {

  // Floating point number may start with a signed integer part
  uintmax_t whole;
  sv = parse_sized_integer(sv, &whole, base, 4);

  if (sv.count == 0) {
    *out = (long double)whole;
    return sv;
  }

  if (!sv_starts_with_char(sv, '.')) {
    *out = (long double)whole;
    return sv;
  }

  // Drop period '.'
  sv = sv_drop(sv, 1);

  // Parse fractional part
  long double frac = 0.0L;
  while (sv.count > 0) {
    int d = digit_of_base(*sv.items, base);
    if (d == -1) {
      break;
    }
    frac += d;
    frac /= base;
    sv = sv_drop(sv, 1);
  }

  if (sv.count == 0) {
    *out = (long double)whole + frac;
    return sv;
  }

  // Parse optional exponent
  long double result = (long double)whole + frac;
  if (base <= 10 && (*sv.items == 'e' || *sv.items == 'E')) {

    // 1.1e10, etc.

    sv = sv_drop(sv, 1);
    if (sv_starts_with_char(sv, '+')) {
      sv = sv_drop(sv, 1);
    }

    intmax_t power;
    sv = parse_signed_sized_integer(sv, &power, 10, 4);

    result *= powl(10, power);

  } else if (base > 10 && *sv.items == 'p') {

    // 0x2a.fp-4, etc

    sv = sv_drop(sv, 1);
    if (sv_starts_with_char(sv, '+')) {
      sv = sv_drop(sv, 1);
    }

    intmax_t power;
    sv = parse_signed_sized_integer(sv, &power, 10, 4);

    result *= powl(2, power);
  }

  *out = result;
  return sv;
}

string_view_t sv_scanf(string_view_t sv, const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  string_view_t result = sv_vscanf(sv, fmt, va);
  va_end(va);
  return result;
}

string_view_t sv_vscanf(string_view_t sv, const char *fmt, va_list va_in) {
  string_view_t vfmt = sv_cstr(fmt);

  va_list va;
  va_copy(va, va_in);

  while (sv.count != 0 && vfmt.count != 0) {

    if (*vfmt.items == '%') {

      format_specifier_t spec;
      int ec = fmt_parse_specifier(&vfmt, &spec);
      assert(ec == 0);

      fmt_fetch_lengths(&spec, &va);

      switch (spec.specifier) {
      case '%':
        if (*sv.items != '%') {
          goto END;
        }
        sv = sv_drop(sv, 1);
        break;
      case 'd':
        if (strcmp(spec.length, "ll") == 0) {
          sv = sv_read_ll(sv, va_arg(va, long long *), 10);
        } else if (strcmp(spec.length, "l") == 0) {
          sv = sv_read_l(sv, va_arg(va, long *), 10);
        } else if (strcmp(spec.length, "j")) {
          sv = sv_read_imax(sv, va_arg(va, intmax_t *), 10);
        } else if (strcmp(spec.length, "t")) {
          sv = sv_read_iz(sv, va_arg(va, ptrdiff_t *), 10);
        } else {
          sv = sv_read_i(sv, va_arg(va, int *), 10);
        }
        break;
      case 'u':
        if (strcmp(spec.length, "ll") == 0) {
          sv = sv_read_ull(sv, va_arg(va, long long *), 10);
        } else if (strcmp(spec.length, "l") == 0) {
          sv = sv_read_ul(sv, va_arg(va, long *), 10);
        } else if (strcmp(spec.length, "j")) {
          sv = sv_read_umax(sv, va_arg(va, intmax_t *), 10);
        } else if (strcmp(spec.length, "z")) {
          sv = sv_read_uz(sv, va_arg(va, size_t *), 10);
        } else {
          sv = sv_read_u(sv, va_arg(va, unsigned int *), 10);
        }
        break;
      case 'x':
      case 'X':
        if (strcmp(spec.length, "ll") == 0) {
          sv = sv_read_ull(sv, va_arg(va, long long *), 16);
        } else if (strcmp(spec.length, "l") == 0) {
          sv = sv_read_ul(sv, va_arg(va, long *), 16);
        } else if (strcmp(spec.length, "j")) {
          sv = sv_read_umax(sv, va_arg(va, intmax_t *), 16);
        } else if (strcmp(spec.length, "z")) {
          sv = sv_read_uz(sv, va_arg(va, size_t *), 16);
        } else if (strcmp(spec.length, "t")) {
          sv = sv_read_uz(sv, va_arg(va, intptr_t *), 16);
        } else {
          sv = sv_read_u(sv, va_arg(va, unsigned int *), 16);
        }
        break;
      case 'o':
      case 'O':
        if (strcmp(spec.length, "ll") == 0) {
          sv = sv_read_ull(sv, va_arg(va, long long *), 8);
        } else if (strcmp(spec.length, "l") == 0) {
          sv = sv_read_ul(sv, va_arg(va, long *), 8);
        } else if (strcmp(spec.length, "j")) {
          sv = sv_read_umax(sv, va_arg(va, intmax_t *), 8);
        } else if (strcmp(spec.length, "z")) {
          sv = sv_read_uz(sv, va_arg(va, size_t *), 8);
        } else if (strcmp(spec.length, "t")) {
          sv = sv_read_uz(sv, va_arg(va, intptr_t *), 8);
        } else {
          sv = sv_read_u(sv, va_arg(va, unsigned int *), 8);
        }
        break;
      case 'g':
      case 'G':
      case 'f':
      case 'F':
        if (strcmp(spec.length, "l") == 0) {
          sv = sv_read_ld(sv, va_arg(va, long double *), 10);
        } else {
          sv = sv_read_d(sv, va_arg(va, double *), 10);
        }
        break;
      case 'a':
      case 'A':
        if (strcmp(spec.length, "l") == 0) {
          sv = sv_read_ld(sv, va_arg(va, long double *), 16);
        } else {
          sv = sv_read_d(sv, va_arg(va, double *), 16);
        }
        break;
      }
      continue;
    }

    if (*vfmt.items != *sv.items) {
      break;
    }

    sv = sv_drop(sv, 1);
    vfmt = sv_drop(vfmt, 1);
  }
END:
  va_end(va);
  return sv;
}
