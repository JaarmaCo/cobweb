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
 * @file stream.h
 *
 * Defines a stream abstraction type that allows for reading and writing bytes
 * using custom callbacks
 */

#if !defined(STREAM_H_)
#define STREAM_H_

#include "string_builder.h"
#include "string_view.h"

#include <stdarg.h>
#include <stdio.h>

typedef struct istream istream_t;
typedef struct ostream ostream_t;

/**
 * Input stream abstraction that permits streaming reads via a callback.
 */
struct istream {
  /**
   * fgetc-like callback that gets the next byte from the input stream. Returns
   * EOF when an error occurs, or if the stream ended.
   */
  int (*get)(void *);

  /**
   * Pointer to pass to the get callback.
   */
  void *user;
};

/**
 * Output stream abstraction that permits writing single byte using a callback.
 */
struct ostream {

  /**
   * fputc-like callback that puts the next byte into the output stream.
   *
   * This function should return a negative value if an error occurs, and any
   * other value on success.
   */
  int (*put)(int, void *);

  /**
   * Pointer to pass to the put callback.
   */
  void *user;
};

/**
 * Gets an input stream that reads from a FILE *
 */
istream_t istream_from_file(FILE *f);

/**
 * Gets an output stream that writes to a FILE *
 */
ostream_t ostream_from_file(FILE *f);

/**
 * Gets an input stream that traverses a string view.
 *
 * @param[inout] sv Pointer to the string view (has to remain alive for the
 *                  lifetime duration of the returned input stream)
 *
 * @return An input stream that reads and advances the given string view
 */
istream_t istream_from_sv(string_view_t *sv);

/**
 * Gets an output stream that writes to a string builder.
 *
 * @param sb Pointer to the string builder structure that is to be filled (has
 *           to remain valid for the lifetime duration of the returned output
 *           stream).
 *
 * @return An output stream that writes by appending characters to the provided
 * string builder.
 */
ostream_t ostream_from_sb(string_builder_t *sb);

/**
 * Gets the next character from the given input stream, or EOF if the stream is
 * exhausted.
 */
int istream_getc(istream_t istream);

/**
 * Reads a line into a static buffer (same spec as fgets).
 *
 * @param[out] str Pointer to the buffer the characters should be written to.
 * @param count Number of characters to read.
 * @param istream The input stream to read from.
 *
 * @return The value of the str parameter on success, NULL if an error occured,
 *         or if no characters where read.
 */
char *istream_gets(char *str, size_t count, istream_t istream);

/**
 * Gets characters into a string builder.
 *
 * @param sb Pointer to the string builder where characters will be written.
 * @param count Maximum number of characters to read.
 * @param istream Input stream to read from.
 *
 * @return The value of sb_cstr(sb), or NULL if an error occured, or if no
 *         characters where read.
 */
char *istream_get_sb(string_builder_t *sb, size_t count, istream_t istream);

/**
 * Same as istream_get_sb, but reads characters until a line-feed character
 * '\n' is read.
 */
char *istream_get_line(string_builder_t *sb, istream_t istream);

/**
 * Same as istream_get_sb, but reads characters until an ASCII whitespace
 * character is reached.
 */
char *istream_get_word(string_builder_t *sb, istream_t istream);

/**
 * Gets characters into a string builder while a predicate is true.
 *
 * @param sb String builder to append to.
 * @param pred Predicate to match for every read character.
 * @param user User-defined argument that is passed to the predicate.
 * @param max_count Maximum number of bytes to read.
 * @param istream Input stream to read from.
 *
 * @{
 */
char *istream_get_while(string_builder_t *sb, int (*pred)(int),
                        istream_t istream);

char *istream_get_while_usr(string_builder_t *sb, int (*pred)(int, void *),
                            void *user, istream_t istream);

char *istream_get_for(string_builder_t *sb, int (*pred)(int), size_t max_count,
                      istream_t istream);

char *istream_get_for_usr(string_builder_t *sb, int (*pred)(int, void *),
                          void *user, size_t max_count, istream_t istream);

/**
 * @}
 */

/**
 * Same spec as fputc
 */
int ostream_putc(int ch, ostream_t ostream);

/**
 * Same spec as fputs
 */
int ostream_puts(const char *str, ostream_t ostream);

/**
 * Writes every character of the string view into the output stream.
 *
 * @return A non-negative number on success, a negative value if an error
 *         occured.
 */
int ostream_put_sv(string_view_t sv, ostream_t ostream);

/**
 * Same spec as printf
 * @{
 */
int ostream_format(ostream_t ostream, const char *fmt, ...);
int ostream_vformat(ostream_t ostream, const char *fmt, va_list va);
/**
 * @}
 */

#if defined(UNITY_BUILD)
#include "stream.c"
#endif
#endif
