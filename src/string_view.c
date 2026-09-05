#include "string_view.h"
#include "format.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>

string_view_t sv_cstr(const char *cstr) {
  return (string_view_t){
      .items = cstr,
      .count = strlen(cstr),
  };
}

string_view_t sv_substr(string_view_t sv, size_t index, size_t count) {
  if (count == 0) {
    return (string_view_t){
        .items = NULL,
        .count = 0,
    };
  }
  assert(index < sv.count);
  assert(index + count <= sv.count);
  return (string_view_t){
      .items = sv.items + index,
      .count = count,
  };
}

string_view_t sv_take(string_view_t sv, size_t count) {
  assert(count <= sv.count);
  return sv_substr(sv, 0, count);
}

string_view_t sv_drop(string_view_t sv, size_t count) {
  assert(count <= sv.count);
  return sv_substr(sv, count, sv.count - count);
}

string_view_t sv_trim_left(string_view_t sv, const char *trim_chars) {
  size_t offset;
  for (offset = 0; offset < sv.count; ++offset) {
    bool match = false;
    for (const char *ch = trim_chars; *ch; ++ch) {
      if (*ch == sv.items[offset]) {
        match = true;
        break;
      }
    }
    if (!match) {
      break;
    }
  }
  return sv_drop(sv, offset);
}

string_view_t sv_trim_right(string_view_t sv, const char *trim_chars) {
  size_t offset;
  for (offset = 0; offset < sv.count; ++offset) {
    bool match = false;
    for (const char *ch = trim_chars; *ch; ++ch) {
      if (*ch == sv.items[sv.count - offset - 1]) {
        match = true;
        break;
      }
    }
    if (!match) {
      break;
    }
  }
  return sv_take(sv, sv.count - offset);
}

string_view_t sv_trim(string_view_t sv, const char *trim_chars) {
  return sv_trim_left(sv_trim_right(sv, trim_chars), trim_chars);
}

size_t sv_find_char(string_view_t sv, char ch, size_t offset) {
  for (size_t i = offset; i < sv.count; ++i) {
    if (sv.items[i] == ch) {
      return i;
    }
  }
  return (size_t)-1;
}

size_t sv_rfind_char(string_view_t sv, char ch, size_t offset) {
  for (size_t i = offset; i < sv.count; ++i) {
    if (sv.items[sv.count - i - 1] == ch) {
      return i;
    }
  }
  return (size_t)-1;
}

size_t sv_find_substr(string_view_t sv, string_view_t substr, size_t offset) {
  if (substr.count + offset > sv.count) {
    return (size_t)-1;
  }
  for (size_t i = offset; i + substr.count < sv.count; ++i) {
    if (strncmp(sv.items + i, substr.items, substr.count) == 0) {
      return i;
    }
  }
  return (size_t)-1;
}

size_t sv_rfind_substr(string_view_t sv, string_view_t substr, size_t offset) {
  if (substr.count + offset > sv.count) {
    return (size_t)-1;
  }
  for (size_t i = offset; i + substr.count < sv.count; ++i) {
    if (strncmp(sv.items + sv.count - i - substr.count, substr.items,
                substr.count) == 0) {
      return i;
    }
  }
  return (size_t)-1;
}

bool sv_starts_with_char(string_view_t sv, char ch) {
  return sv.count > 0 && sv.items[0] == ch;
}

bool sv_starts_with_substr(string_view_t sv, string_view_t substr) {
  return sv.count >= substr.count &&
         strncmp(sv.items, substr.items, substr.count) == 0;
}

bool sv_ends_with_char(string_view_t sv, char ch) {
  return sv.count > 0 && sv.items[sv.count - 1] == ch;
}

bool sv_ends_with_substr(string_view_t sv, string_view_t substr) {
  return sv.count >= substr.count && strncmp(sv.items + sv.count - substr.count,
                                             substr.items, substr.count) == 0;
}

bool sv_equals(string_view_t lhs, string_view_t rhs) {
  return lhs.count == rhs.count &&
         strncmp(lhs.items, rhs.items, lhs.count) == 0;
}

int sv_compare(string_view_t lhs, string_view_t rhs) {
  if (lhs.count == rhs.count) {
    return strncmp(lhs.items, rhs.items, lhs.count);
  } else if (lhs.count < rhs.count) {
    int cmp = strncmp(lhs.items, rhs.items, lhs.count);
    if (cmp != 0) {
      return cmp;
    }
    return -1;
  } else {
    int cmp = strncmp(lhs.items, rhs.items, rhs.count);
    if (cmp != 0) {
      return cmp;
    }
    return 1;
  }
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

int32_t sv_decode_utf8(string_view_t *sv) {

  if (sv->count == 0) {
    return -1;
  }

  int32_t result = -1;
  if ((sv->items[0] & ~0x7F) == 0x00) {

    result = sv->items[0];
    *sv = sv_drop(*sv, 1);

  } else if ((sv->items[0] & ~0x1F) == 0xC0) {

    if (sv->count < 2) {
      return -1;
    }

    if ((sv->items[1] & ~0x3F) != 0x80) {
      return -1;
    }

    result = (sv->items[0] & 0x1F) << 6 | (sv->items[1] & 0x3F);
    *sv = sv_drop(*sv, 2);

  } else if ((sv->items[0] & ~0x0F) == 0xE0) {

    if (sv->count < 3) {
      return -1;
    }

    if ((sv->items[1] & ~0x3F) != 0x80) {
      return -1;
    }

    if ((sv->items[2] & ~0x3F) != 0x80) {
      return -1;
    }

    result = (sv->items[0] & 0x0F) << 12 | (sv->items[1] & 0x3F) << 6 |
             (sv->items[2] & 0x3F);
    *sv = sv_drop(*sv, 3);

  } else if ((sv->items[0] & ~0x07) != 0xF0) {

    if (sv->count < 4) {
      return -1;
    }

    if ((sv->items[1] & ~0x3F) != 0x80) {
      return -1;
    }

    if ((sv->items[2] & ~0x3F) != 0x80) {
      return -1;
    }

    if ((sv->items[3] & ~0x3F) != 0x80) {
      return -1;
    }

    result = (sv->items[0] & 0x07) << 18 | (sv->items[1] & 0x3F) << 12 |
             (sv->items[2] & 0x3F) << 6 | (sv->items[3] & 0x3F);
    *sv = sv_drop(*sv, 4);
  }
  return result;
}
