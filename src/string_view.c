#if !defined(UNITY_BUILD)
#include "string_view.h"
#endif

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

string_view_t sv_group(string_view_t *sv, string_view_t left,
                       string_view_t right) {

  size_t pos = sv_find_substr(*sv, left, 0);
  if (pos == (size_t)-1) {
    return (string_view_t){0};
  }
  pos += left.count;

  size_t region_begin = pos;
  size_t region_count = 0;
  int balance = 1;
  while (balance != 0 && pos < sv->count) {

    string_view_t current = sv_drop(*sv, pos);
    if (sv_starts_with_substr(current, left)) {
      ++balance;
      pos += left.count;
      region_count += left.count;
    } else if (sv_starts_with_substr(current, right)) {
      --balance;
      pos += right.count;
      region_count += right.count;
    } else {
      ++pos;
      region_count += 1;
    }
  }

  if (balance != 0) {
    return (string_view_t){0};
  }

  string_view_t source = *sv;
  *sv = sv_drop(source, pos);
  return sv_substr(source, region_begin, region_count - right.count);
}

string_view_t sv_take(string_view_t sv, size_t count) {
  assert(count <= sv.count);
  return sv_substr(sv, 0, count);
}

string_view_t sv_take_while(string_view_t sv, int (*pred)(int)) {
  size_t i;
  for (i = 0; i < sv.count; ++i) {
    if (!pred(sv.items[i])) {
      break;
    }
  }
  return sv_take(sv, i);
}

string_view_t sv_take_while_usr(string_view_t sv, int (*pred)(int, void *),
                                void *user) {
  size_t i;
  for (i = 0; i < sv.count; ++i) {
    if (!pred(sv.items[i], user)) {
      break;
    }
  }
  return sv_take(sv, i);
}

string_view_t sv_take_for(string_view_t sv, int (*pred)(int), size_t count) {
  size_t i;
  for (i = 0; i < sv.count && i < count; ++i) {
    if (!pred(sv.items[i])) {
      break;
    }
  }
  return sv_take(sv, count);
}

string_view_t sv_take_for_usr(string_view_t sv, int (*pred)(int, void *),
                              void *user, size_t count) {
  size_t i;
  for (i = 0; i < sv.count && i < count; ++i) {
    if (!pred(sv.items[i], user)) {
      break;
    }
  }
  return sv_take(sv, count);
}

string_view_t sv_drop(string_view_t sv, size_t count) {
  assert(count <= sv.count);
  return sv_substr(sv, count, sv.count - count);
}

string_view_t sv_drop_while(string_view_t sv, int (*pred)(int)) {
  size_t i;
  for (i = 0; i < sv.count; ++i) {
    if (!pred(sv.items[i])) {
      break;
    }
  }
  return sv_drop(sv, i);
}

string_view_t sv_drop_while_usr(string_view_t sv, int (*pred)(int, void *),
                                void *user) {
  size_t i;
  for (i = 0; i < sv.count; ++i) {
    if (!pred(sv.items[i], user)) {
      break;
    }
  }
  return sv_drop(sv, i);
}

string_view_t sv_drop_for(string_view_t sv, int (*pred)(int), size_t count) {
  size_t i;
  for (i = 0; i < sv.count && i < count; ++i) {
    if (!pred(sv.items[i])) {
      break;
    }
  }
  return sv_drop(sv, count);
}

string_view_t sv_drop_for_usr(string_view_t sv, int (*pred)(int, void *),
                              void *user, size_t count) {
  size_t i;
  for (i = 0; i < sv.count && i < count; ++i) {
    if (!pred(sv.items[i], user)) {
      break;
    }
  }
  return sv_drop(sv, count);
}

string_view_t sv_cut_ch(string_view_t *sv, char delim) {

  size_t pos = sv_find_char(*sv, delim, 0);
  if (pos == (size_t)-1) {

    string_view_t result = *sv;
    *sv = (string_view_t){0};
    return result;
  }

  string_view_t result = sv_take(*sv, pos);
  *sv = sv_drop(*sv, pos + 1);
  return result;
}

string_view_t sv_cut_substr(string_view_t *sv, string_view_t delim) {

  size_t pos = sv_find_substr(*sv, delim, 0);
  if (pos == (size_t)-1) {

    string_view_t result = *sv;
    *sv = (string_view_t){0};
    return result;
  }

  string_view_t result = sv_take(*sv, pos);
  *sv = sv_drop(*sv, pos + delim.count);
  return result;
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
      return sv.count - i - 1;
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
      return sv.count - i - substr.count;
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

size_t sv_hash(string_view_t sv) {
#define FNV1A_PRIME UINT64_C(0x00000100000001b3)
#define FNV1A_OFFSET UINT64_C(0xcbf29ce484222325)

  uint64_t hash = FNV1A_OFFSET;
  for (size_t i = 0; i < sv.count; ++i) {
    hash *= FNV1A_PRIME;
    hash ^= sv.items[i];
  }
  return (size_t)hash;
}

bool sv_subrange_of(string_view_t sv, string_view_t other) {
  const char *src_beg = sv.items, *src_end = sv.items + sv.count;
  const char *oth_beg = other.items, *oth_end = other.items + other.count;
  return oth_beg - src_beg >= 0 && src_end - oth_end >= 0;
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
