#if !defined(STRING_VIEW_H_)

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
 * Gets a substring of the string-view that starts at the beginning of the
 * source string and contains count characters.
 *
 * @param count Number of characters in the substring.
 */
string_view_t sv_take(string_view_t sv, size_t count);

/**
 * Gets a substring that starts count characters from the start of the source
 * string, and contains all remaining characters.
 *
 * @param count Number of characters to exclude from the substring.
 */
string_view_t sv_drop(string_view_t sv, size_t count);

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

/**
 * Decodes the next UTF-8 codepoint from the specified string-view, then cuts
 * the view so that the characters of the decoded codepoint are removed.
 *
 * If an encoding error occurs, -1 is returned, and the string view is not
 * modified.
 *
 * @param[inout] sv View to extract a codepoint from, is set to the remaining
 *                  view after the codepoint when this function returns.
 * @return The decoded UTF-8 codepoint, or -1 if an encoding error occured.
 */
int32_t sv_decode_utf8(string_view_t *sv);

#define STRING_VIEW_H_
#endif
