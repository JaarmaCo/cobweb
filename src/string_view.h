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
 * @file string_view.h
 *
 * Defines the string_view_t type, an unmanaged pointer to a string slice, as
 * well as a set of utilities for string processing.
 */

#if !defined(STRING_VIEW_H_)
#define STRING_VIEW_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Constructs a string view from a string literal.
 */
#define SV(str) ((string_view_t){.items = (str), .count = sizeof(str) - 1})

/**
 * Unmanaged string view type.
 */
typedef struct string_view {

  /**
   * Pointer to the first character in the view.
   */
  const char *items;

  /**
   * Number of elements in the view.
   */
  size_t count;

} string_view_t;

/**
 * Constructs a string-view from a NUL-terminated string.
 */
string_view_t sv_cstr(const char *cstr);

/**
 * Gets a substring of a string-view.
 *
 * @param index Index of the first character in the substring.
 * @param count Number of characters in the substring.
 */
string_view_t sv_substr(string_view_t sv, size_t index, size_t count);

/**
 * Read a region that exists inside two delimiters left and right (I.e read a
 * parenthesized argument).
 *
 * When the function returns, the in-group string is the returned value, and the
 * view sv points to the string after the group argument.
 *
 * @param[inout] sv Variable containing the view to advance.
 * @param left Substring that opens a new group.
 * @param right Substring that closes a group.
 * @return The string containing the first group delimitered by left, right
 */
string_view_t sv_group(string_view_t *sv, string_view_t left,
                       string_view_t right);

/**
 * Gets a substring of the string-view that starts at the beginning of the
 * source string and contains count characters. If a predicate is provided,
 * only the characters that match the predicate are included.
 *
 * @param count Number of characters in the substring.
 * @param pred Predicate that has to match every taken character.
 * @param user User-defined argument to pass to the predicate.
 * @{
 */
// clang-format off
string_view_t sv_take(string_view_t sv, size_t count);
string_view_t sv_take_while(string_view_t sv, int (*pred)(int));
string_view_t sv_take_while_usr(string_view_t sv, int (*pred)(int, void *), void *user);
string_view_t sv_take_for(string_view_t sv, int (*pred)(int), size_t count);
string_view_t sv_take_for_usr(string_view_t sv, int (*pred)(int, void *), void *user, size_t count);
/**
 * @}
 */

/**
 * Gets a substring that starts count characters from the start of the source
 * string, and contains all remaining characters. If a predicate is provided,
 * only the characters matching the predicate are skipped.
 *
 * @param count Number of characters to exclude from the substring.
 * @param pred Predicate to match for every dropped character.
 * @param user User-defind argument to pass to the predicate.
 * @{
 */
string_view_t sv_drop(string_view_t sv, size_t count);
string_view_t sv_drop_while(string_view_t sv, int(*pred)(int));
string_view_t sv_drop_while_usr(string_view_t sv, int(*pred)(int, void *), void *user);
string_view_t sv_drop_for(string_view_t sv, int(*pred)(int), size_t count);
string_view_t sv_drop_for_usr(string_view_t sv, int(*pred)(int, void *), void *user, size_t count);
// clang-format on
/**
 * @}
 */

/**
 * Cut a string by a delimiter. This function returns the part of the string,
 * before the delimiter, and the view the sv argument points to is adjusted so
 * it points to the string after the delimiter.
 *
 * @param[inout] sv Pointer to a string view to cut, and output parameter for
 *                  the cut suffix.
 * @param delim Delimiter to cut by
 * @return A view into the string before the delimiter.
 * @{
 */
string_view_t sv_cut_ch(string_view_t *sv, char delim);
string_view_t sv_cut_substr(string_view_t *sv, string_view_t delim);
/**
 * @}
 */

/**
 * Searches the string-view from the left and removes every occurence of
 * trim_chars. Stops if a character not in trim_chars is encountered.
 *
 * @param trim_chars NUL-terminated string containing the characters to remove.
 */
string_view_t sv_trim_left(string_view_t sv, const char *trim_chars);

/**
 * Searches the string-view from the right and removes every occurence of
 * trim_chars. Stops if a character not in trim_chars is encountered.
 *
 * @param trim_chars NUL-terminated string containing the characters to remove.
 */
string_view_t sv_trim_right(string_view_t sv, const char *trim_chars);

/**
 * Trim the string from both the left and right side.
 *
 * @param trim_chars NUL-terminated string containing the characters to remove.
 */
string_view_t sv_trim(string_view_t sv, const char *trim_chars);

/**
 * Finds the first occurence of the specified character in the string-view,
 * starting from a specified offset.
 *
 * @param ch Character to search for.
 * @param offset Offset to start searching from.
 * @return The index of the character, or (size_t)-1 if no match was found.
 */
size_t sv_find_char(string_view_t sv, char ch, size_t offset);

/**
 * Finds the last occurence of the specified character in the string-view by
 * searching the string-view from the end towards the beginning.
 *
 * @param ch Character to search for.
 * @param offset Offset from the last character in the string to start searching
 *               from.
 * @return The index of the located character, or (size_t)-1 if no match was
 *         found.
 */
size_t sv_rfind_char(string_view_t sv, char ch, size_t offset);

/**
 * Finds the first occurence of the specified substring in the string-view,
 * starting from a specified offset.
 *
 * @param substr Substring to search for.
 * @param offset Offset to start searching from.
 * @return The index of the first character in the located substring, or
 *         (size_t)-1 if no match was found.
 */
size_t sv_find_substr(string_view_t sv, string_view_t substr, size_t offset);

/**
 * Finds the last occurence of the specified substring in the string-view by
 * searching the string-view from the end towards the beginning.
 *
 * @param substr Substring to search for.
 * @param offset Offset from the last character in the string to start searching
 *               from.
 * @return The index of the first character in the located substring, or
 *         (size_t)-1 if no match was found.
 */
size_t sv_rfind_substr(string_view_t sv, string_view_t substr, size_t offset);

/**
 * Checks whether the string starts with the given character.
 */
bool sv_starts_with_char(string_view_t sv, char ch);

/**
 * Checks whether the string starts with the specified substring.
 */
bool sv_starts_with_substr(string_view_t sv, string_view_t substr);

/**
 * Checks whether the string ends with the given character.
 */
bool sv_ends_with_char(string_view_t sv, char ch);

/**
 * Checks whether the string ends with the specified substring.
 */
bool sv_ends_with_substr(string_view_t sv, string_view_t substr);

/**
 * Checks whether sv_find_char can find an occurence of the given character.
 */
static inline bool sv_contains_char(string_view_t sv, char ch) {
  return sv_find_char(sv, ch, 0) != (size_t)-1;
}

/**
 * Checks whether sv_find_substr can find an occurence of the given substring.
 */
static inline bool sv_contains_substr(string_view_t sv, string_view_t substr) {
  return sv_find_substr(sv, substr, 0) != (size_t)-1;
}

/**
 * Checks if the left and right string are of the same length and contains the
 * same characters.
 */
bool sv_equals(string_view_t lhs, string_view_t rhs);

/**
 * Gets the hash of a string view.
 */
size_t sv_hash(string_view_t sv);

/**
 * Tests whether the other string view is a view into the same array as, and
 * within the given source view.
 *
 * @param sv Source view to treat as the superset.
 * @param substr String to check.
 *
 * @return true if substr is a substring of sv, and part of the same underlying
 *         character array.
 */
bool sv_subrange_of(string_view_t sv, string_view_t substr);

/**
 * Lexicographically compares the left and right strings.
 *
 * If one string was longer than the other, and compared equal in all characters
 * up until the point where the other string terminated; the longer string is
 * considered greater than the shorter string.
 */
int sv_compare(string_view_t lhs, string_view_t rhs);

/**
 * scanf-like function that parses input from the given string-view according to
 * a format specifier.
 *
 * @param sv String-view to scan for input.
 * @param fmt C-format specifier.
 * @param va Variadic output arguments.
 * @return The part of the string-view that was not matched by the format
 *         specifier.
 * @{
 */
string_view_t sv_scanf(string_view_t sv, const char *fmt, ...);
string_view_t sv_vscanf(string_view_t sv, const char *fmt, va_list va);
/**
 * @}
 */

/**
 * Read a numeric value from the start of the string view.
 *
 * @param sv View to parse a number from.
 * @param out Pointer to a variable that receives the parsed value.
 * @param base Numeric base to use, or 0 to infer the base.
 * @return The remainder of the string view after the value was parsed.
 * @{
 */

string_view_t sv_read_u8(string_view_t sv, uint8_t *out, int base);

string_view_t sv_read_u16(string_view_t sv, uint16_t *out, int base);

string_view_t sv_read_u32(string_view_t sv, uint32_t *out, int base);

string_view_t sv_read_u64(string_view_t sv, uint64_t *out, int base);

string_view_t sv_read_i8(string_view_t sv, int8_t *out, int base);

string_view_t sv_read_i16(string_view_t sv, int16_t *out, int base);

string_view_t sv_read_i32(string_view_t sv, int32_t *out, int base);

string_view_t sv_read_i64(string_view_t sv, int64_t *out, int base);

string_view_t sv_read_imax(string_view_t sv, intmax_t *out, int base);

string_view_t sv_read_umax(string_view_t sv, uintmax_t *out, int base);

string_view_t sv_read_uz(string_view_t sv, size_t *out, int base);

string_view_t sv_read_iz(string_view_t sv, ptrdiff_t *out, int base);

string_view_t sv_read_iptr(string_view_t sv, intptr_t *out, int base);

string_view_t sv_read_uptr(string_view_t sv, uintptr_t *out, int base);

string_view_t sv_read_i(string_view_t sv, int *out, int base);

string_view_t sv_read_l(string_view_t sv, long *out, int base);

string_view_t sv_read_ll(string_view_t sv, long long *out, int base);

string_view_t sv_read_u(string_view_t sv, unsigned int *out, int base);

string_view_t sv_read_ul(string_view_t sv, unsigned long *out, int base);

string_view_t sv_read_ull(string_view_t sv, unsigned long long *out, int base);

string_view_t sv_read_f(string_view_t sv, float *out, int base);

string_view_t sv_read_d(string_view_t sv, double *out, int base);

string_view_t sv_read_ld(string_view_t sv, long double *out, int base);

/**
 * @}
 */

#if defined(UNITY_BUILD)
#include "string_view.c"
#include "sv_read.c"
#endif
#endif
