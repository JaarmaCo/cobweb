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
#include "stream.h"
#endif

#include <ctype.h>
#include <errno.h>
#include <string.h>

istream_t istream_from_file(FILE *f) {
  return (istream_t){
      .get = (int (*)(void *))fgetc,
      .user = f,
  };
}

ostream_t ostream_from_file(FILE *f) {
  return (ostream_t){
      .put = (int (*)(int, void *))fputc,
      .user = f,
  };
}

static int get_sv(void *user) {
  string_view_t *sv = user;
  if (sv->count == 0) {
    return EOF;
  }
  int ch = *sv->items;
  *sv = sv_drop(*sv, 1);
  return ch;
}

istream_t istream_from_sv(string_view_t *sv) {
  return (istream_t){
      .get = get_sv,
      .user = sv,
  };
}

static int put_sb(int ch, void *user) {
  string_builder_t *sb = user;
  if (ch < 0 || ch > 0xff) {
    errno = EIO;
    perror("Character out of range");
    return EOF;
  }
  if (!sb_append_char(sb, (char)ch)) {
    errno = ENOMEM;
    perror("sb_append_char");
    return EOF;
  }
  return (unsigned char)ch;
}

ostream_t ostream_from_sb(string_builder_t *sb) {
  return (ostream_t){
      .put = put_sb,
      .user = sb,
  };
}

int istream_getc(istream_t istream) {
  if (!istream.get) {
    errno = EIO;
    perror("istream.get is not defined");
    return EOF;
  }
  return istream.get(istream.user);
}

char *istream_gets(char *str, size_t count, istream_t istream) {

  if (count == 0) {
    return str;
  }

  size_t i;
  for (i = 0; i + 1 < count; ++i) {
    int ch = istream_getc(istream);

    if (ch < 0 || ch > 0xff) {
      break;
    }

    if (ch == '\n') {
      str[i++] = '\n';
      break;
    }

    str[i] = (char)ch;
  }

  str[i] = 0;
  return i == 0 ? NULL : str;
}

char *istream_get_sb(string_builder_t *sb, size_t count, istream_t istream) {

  if (count == 0) {
    return sb->items;
  }

  size_t i;
  for (i = 0; i < count; ++i) {
    int ch = istream_getc(istream);
    if (ch < 0 || ch > 0xff) {
      break;
    }

    if (!sb_append_char(sb, (char)ch)) {
      return NULL;
    }
  }
  return i == 0 ? NULL : sb->items;
}

char *istream_get_line(string_builder_t *sb, istream_t istream) {

  size_t i;
  for (i = 0;; ++i) {
    int ch = istream_getc(istream);
    if (ch < 0 || ch > 0xff) {
      break;
    }

    if (!sb_append_char(sb, (char)ch)) {
      return NULL;
    }

    if (ch == '\n') {
      break;
    }
  }
  return i == 0 ? NULL : sb->items;
}

char *istream_get_word(string_builder_t *sb, istream_t istream) {

  size_t i;
  for (i = 0;; ++i) {
    int ch = istream_getc(istream);
    if (ch < 0 || ch > 0xff) {
      break;
    }

    if (isspace(ch)) {
      break;
    }

    if (!sb_append_char(sb, (char)ch)) {
      return NULL;
    }
  }
  return i == 0 ? NULL : sb->items;
}

char *istream_get_while(string_builder_t *sb, int (*pred)(int),
                        istream_t istream) {
  size_t i;
  for (i = 0;; ++i) {
    int ch = istream_getc(istream);
    if (ch < 0 || ch > 0xff) {
      break;
    }

    if (!pred(ch)) {
      break;
    }

    if (!sb_append_char(sb, (char)ch)) {
      return NULL;
    }
  }
  return i == 0 ? NULL : sb->items;
}

char *istream_get_while_usr(string_builder_t *sb, int (*pred)(int, void *),
                            void *user, istream_t istream) {
  size_t i;
  for (i = 0;; ++i) {
    int ch = istream_getc(istream);
    if (ch < 0 || ch > 0xff) {
      break;
    }

    if (!pred(ch, user)) {
      break;
    }

    if (!sb_append_char(sb, (char)ch)) {
      return NULL;
    }
  }
  return i == 0 ? NULL : sb->items;
}

char *istream_get_for(string_builder_t *sb, int (*pred)(int), size_t max_count,
                      istream_t istream) {
  size_t i;
  for (i = 0; i < max_count; ++i) {
    int ch = istream_getc(istream);
    if (ch < 0 || ch > 0xff) {
      break;
    }

    if (!pred(ch)) {
      break;
    }

    if (!sb_append_char(sb, (char)ch)) {
      return NULL;
    }
  }
  return i == 0 ? NULL : sb->items;
}

char *istream_get_for_usr(string_builder_t *sb, int (*pred)(int, void *),
                          void *user, size_t max_count, istream_t istream) {
  size_t i;
  for (i = 0; i < max_count; ++i) {
    int ch = istream_getc(istream);
    if (ch < 0 || ch > 0xff) {
      break;
    }

    if (!pred(ch, user)) {
      break;
    }

    if (!sb_append_char(sb, (char)ch)) {
      return NULL;
    }
  }
  return i == 0 ? NULL : sb->items;
}

int ostream_putc(int ch, ostream_t ostream) {
  if (!ostream.put) {
    perror("ostream.put is not defined");
    errno = EIO;
    return EOF;
  }
  return ostream.put(ch, ostream.user);
}

int ostream_puts(const char *str, ostream_t ostream) {
  for (const char *ptr = str; *ptr; ++ptr) {
    if (ostream_putc(*ptr, ostream) < 0) {
      return EOF;
    }
  }
  return ostream_putc('\n', ostream);
}

int ostream_put_sv(string_view_t sv, ostream_t ostream) {
  for (size_t i = 0; i < sv.count; ++i) {
    if (ostream_putc(sv.items[i], ostream) < 0) {
      return EOF;
    }
  }
  return 0;
}

int ostream_format(ostream_t ostream, const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  int ec = ostream_vformat(ostream, fmt, va);
  va_end(va);
  return ec;
}

int ostream_vformat(ostream_t ostream, const char *fmt, va_list va) {
  if (ostream.put == (int (*)(int, void *))fputc) {
    return vfprintf(ostream.user, fmt, va);
  } else if (ostream.put == put_sb) {
    return sb_vformat(ostream.user, fmt, va) ? 0 : EOF;
  }
  errno = EIO;
  perror("ostream_vformat not implemented for this stream type");
  return EOF;
}
