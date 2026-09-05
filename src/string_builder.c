#include "string_builder.h"

string_view_t sb_view(const string_builder_t *sb) {
  return (string_view_t){
      .items = sb->items,
      .count = sb->count == 0 ? 0 : sb->count - 1,
  };
}

string_view_t sb_canonicalize_view(string_builder_t *sb, string_view_t view) {
  string_view_t sbv = sb_view(sb);
  if (!sv_subrange_of(sbv, view)) {
    return view;
  }
  memmove(sb->items, view.items, view.count);
  sb->items[view.count] = 0;
  sb->count = view.count + 1;
  return sb_view(sb);
}

char *sb_append_char(string_builder_t *sb, char ch) {

  if (sb->count < 1) {
    if (!da_append_c(sb, 0)) {
      return NULL;
    }
  }

  if (!da_append_c(sb, 0)) {
    return NULL;
  }

  sb->items[sb->count - 2] = ch;
  return sb->items;
}

char *sb_append_sv(string_builder_t *sb, string_view_t sv) {
  size_t nc = da_reserve_c(sb, sb->count + sv.count + 1);
  if (nc == 0) {
    return NULL;
  }
  strncpy(sb->items + sb->count, sv.items, sv.count);
  sb->count += sv.count + 1;
  return sb->items;
}

char *sb_append_cstr(string_builder_t *sb, const char *cstr) {
  return sb_append_sv(sb, sv_cstr(cstr));
}

char *sb_format(string_builder_t *sb, const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  char *result = sb_vformat(sb, fmt, va);
  va_end(va);
  return result;
}

char *sb_vformat(string_builder_t *sb, const char *fmt, va_list va) {
  for (;;) {

    size_t write_limit = sb->capacity - sb->count + 1;
    int nw = vsnprintf(sb->items + sb->count - 1, write_limit, fmt, va);

    if (nw < 0) {
      return NULL;
    }

    if (nw < (int)write_limit) {
      sb->count += (size_t)nw + 1;
      break;
    }

    if (!da_reserve_c(sb, sb->count + (size_t)nw)) {
      return NULL;
    }
  }
  return sb->items;
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
