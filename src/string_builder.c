#include "string_builder.h"

#include <ctype.h>

string_view_t sb_view(const string_builder_t *sb) {
  return (string_view_t){
      .items = sb->items,
      .count = sb->count,
  };
}

string_view_t sb_canonicalize_view(string_builder_t *sb, string_view_t view) {
  string_view_t sbv = sb_view(sb);
  if (!sv_subrange_of(sbv, view)) {
    return view;
  }
  memmove(sb->items, view.items, view.count);
  sb->items[view.count] = 0;
  sb->count = view.count;
  return sb_view(sb);
}

char *sb_append_char(string_builder_t *sb, char ch) {
  if (!da_reserve_c(sb, sb->count + 2)) {
    return NULL;
  }
  sb->items[sb->count++] = ch;
  sb->items[sb->count] = 0;
  return sb->items;
}

char *sb_append_sv(string_builder_t *sb, string_view_t sv) {
  if (!da_reserve_c(sb, sb->count + sv.count + 1)) {
    return NULL;
  }
  strncpy(sb->items + sb->count, sv.items, sv.count);
  sb->count += sv.count;
  return sb->items;
}

char *sb_append_cstr(string_builder_t *sb, const char *cstr) {
  for (const char *ptr = cstr; *ptr; ++ptr) {
    if (!da_reserve_c(sb, sb->count + 2)) {
      return NULL;
    }
    sb->items[sb->count++] = *ptr;
  }
  sb->items[sb->count] = 0;
  return sb->items;
}

char *sb_append_string_literal(string_builder_t *sb, string_view_t literal,
                               int dialect) {
  (void)dialect;

  if (!da_reserve_c(sb, literal.count + 3)) {
    return NULL;
  }

  if (!sb_append_char(sb, '"')) {
    return NULL;
  }

  for (size_t i = 0; i < literal.count; ++i) {
    switch (literal.items[i]) {
    case '\'':
      if (!sb_append_sv(sb, SV("\\'"))) {
        return NULL;
      }
      break;
    case '\"':
      if (!sb_append_sv(sb, SV("\\\""))) {
        return NULL;
      }
      break;
    case '\?':
      if (!sb_append_sv(sb, SV("\\?"))) {
        return NULL;
      }
      break;
    case '\\':
      if (!sb_append_sv(sb, SV("\\\\"))) {
        return NULL;
      }
      break;
    case '\a':
      if (!sb_append_sv(sb, SV("\\a"))) {
        return NULL;
      }
      break;
    case '\b':
      if (!sb_append_sv(sb, SV("\\b"))) {
        return NULL;
      }
      break;
    case '\f':
      if (!sb_append_sv(sb, SV("\\f"))) {
        return NULL;
      }
      break;
    case '\n':
      if (!sb_append_sv(sb, SV("\\n"))) {
        return NULL;
      }
      break;
    case '\r':
      if (!sb_append_sv(sb, SV("\\r"))) {
        return NULL;
      }
      break;
    case '\t':
      if (!sb_append_sv(sb, SV("\\t"))) {
        return NULL;
      }
      break;
    case '\v':
      if (!sb_append_sv(sb, SV("\\v"))) {
        return NULL;
      }
      break;
    default:
      if (isprint(literal.items[i])) {
        if (!sb_append_char(sb, literal.items[i])) {
          return NULL;
        }
        break;
      }

      if (!sb_format(sb, "\\x%0.2x", (unsigned char)literal.items[i])) {
        return NULL;
      }
      break;
    }
  }

  if (!sb_append_char(sb, '"')) {
    return NULL;
  }

  return sb->items;
}

char *sb_format(string_builder_t *sb, const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  char *result = sb_vformat(sb, fmt, va);
  va_end(va);
  return result;
}

char *sb_vformat(string_builder_t *sb, const char *fmt, va_list va) {

  for (int i = 0; i < 2; ++i) {

    va_list cp;
    va_copy(cp, va);

    size_t remaining_capacity = sb->capacity - sb->count;
    int nw = vsnprintf(sb->items + sb->count, remaining_capacity, fmt, cp);

    va_end(cp);

    if (nw < 0) { // Format error
      return NULL;
    }

    if ((size_t)nw < remaining_capacity) { // Complete write
      sb->count += (size_t)nw;
      return sb->items;
    }

    // Insufficient space, reserve and try again
    if (!da_reserve_c(sb, sb->count + (size_t)nw + 1)) {
      return NULL;
    }
  }
  return NULL;
}

char *sb_join_sv(string_builder_t *sb, string_view_t delim, size_t count,
                 const string_view_t *items) {
  for (size_t i = 0; i < count; ++i) {
    if (i != 0) {
      if (!sb_append_sv(sb, delim)) {
        return NULL;
      }
    }
    if (!sb_append_sv(sb, items[i])) {
      return NULL;
    }
  }
  return sb->items;
}

char *sb_join_cstr(string_builder_t *sb, string_view_t delim, size_t count,
                   const char **items) {
  for (size_t i = 0; i < count; ++i) {
    if (i != 0) {
      if (!sb_append_sv(sb, delim)) {
        return NULL;
      }
    }
    if (!sb_append_cstr(sb, items[i])) {
      return NULL;
    }
  }
  return sb->items;
}

char *sb_join_sb(string_builder_t *sb, string_view_t delim, size_t count,
                 const string_builder_t *items) {
  for (size_t i = 0; i < count; ++i) {
    if (i != 0) {
      if (!sb_append_sv(sb, delim)) {
        return NULL;
      }
    }
    if (!sb_append_sv(sb, sb_view(&items[i]))) {
      return NULL;
    }
  }
  return sb->items;
}

char *sb_read_file(string_builder_t *sb, FILE *f, size_t max_read) {

  if (max_read != SIZE_MAX) {
    if (!da_reserve_c(sb, sb->count + max_read + 1)) {
      return NULL;
    }
  }

  for (size_t i = 0; i < max_read; ++i) {

    int ch = fgetc(f);
    if (ch == EOF) {
      break;
    }

    if (!sb_append_char(sb, (char)ch)) {
      return NULL;
    }
  }
  return sb->items;
}

char *sb_read_line(string_builder_t *sb, FILE *f) {

  for (;;) {

    int ch = fgetc(f);
    if (ch == EOF || ch == '\n') {
      break;
    }

    if (!sb_append_char(sb, (char)ch)) {
      return NULL;
    }
  }
  return sb->items;
}

char *sb_read_until(string_builder_t *sb, FILE *f, string_view_t delim) {

  int ch;
  size_t delim_pos = 0;
  for (;;) {

    ch = fgetc(f);
    if (ch == EOF) {
      // There may be deferred characters from a partial delimiter match, append
      // them.
      //
      if (delim_pos != 0 && !sb_append_sv(sb, sv_take(delim, delim_pos))) {
        return NULL;
      }
      break;
    }

    if (ch == delim.items[delim_pos]) {
      // Partial delimiter match, defer appending characters until a non-match.
      //
      ++delim_pos;
    } else {

      // Mismatch: append any deferred characters, also append the mismatched
      // character and reset the delimiter match position.
      //
      if (delim_pos != 0 && !sb_append_sv(sb, sv_take(delim, delim_pos))) {
        return NULL;
      }

      if (!sb_append_char(sb, (char)ch)) {
        return NULL;
      }

      delim_pos = 0;
    }

    // Full delimiter match found, exit.
    //
    if (delim_pos == delim.count) {
      break;
    }
  }
  return sb->items;
}
