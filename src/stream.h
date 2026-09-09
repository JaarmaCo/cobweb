#if !defined(STREAM_H_)
#define STREAM_H_

#include "string_builder.h"
#include "string_view.h"

#include <stdarg.h>
#include <stdio.h>

typedef struct istream istream_t;
typedef struct ostream ostream_t;

struct istream {
  int (*get)(void *);
  void *user;
};

struct ostream {
  int (*put)(int, void *);
  void *user;
};

istream_t istream_from_file(FILE *f);

ostream_t ostream_from_file(FILE *f);

istream_t istream_from_sv(string_view_t *sv);

ostream_t ostream_from_sb(string_builder_t *sb);

int istream_getc(istream_t istream);

char *istream_gets(char *str, size_t count, istream_t istream);

char *istream_get_sb(string_builder_t *sb, size_t count, istream_t istream);

char *istream_get_line(string_builder_t *sb, istream_t istream);

char *istream_get_word(string_builder_t *sb, istream_t istream);

char *istream_get_while(string_builder_t *sb, int (*pred)(int),
                        istream_t istream);

char *istream_get_while_usr(string_builder_t *sb, int (*pred)(int, void *),
                            void *user, istream_t istream);

char *istream_get_for(string_builder_t *sb, int (*pred)(int), size_t max_count,
                      istream_t istream);

char *istream_get_for_usr(string_builder_t *sb, int (*pred)(int, void *),
                          void *user, size_t max_count, istream_t istream);

int ostream_putc(int ch, ostream_t ostream);

int ostream_puts(const char *str, ostream_t ostream);

int ostream_put_sv(string_view_t sv, ostream_t ostream);

int ostream_format(ostream_t ostream, const char *fmt, ...);

int ostream_vformat(ostream_t ostream, const char *fmt, va_list va);

#endif
