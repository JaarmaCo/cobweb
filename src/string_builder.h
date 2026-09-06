#if !defined(STRING_BUILDER_H_)

#include <stdio.h>

#include "dynamic_array_char.h"
#include "string_view.h"

#define STRING_LITERAL_REQUEST_START_QUOTE -1
#define STRING_LITERAL_REQUEST_END_QUOTE -2

/**
 * Type for dynamically constructing NUL-terminated strings.
 */
typedef dynamic_array_c string_builder_t;

/**
 * Deallocate any memory owned by the string builder.
 */
static inline void sb_destroy(string_builder_t *sb) { da_destroy_c(sb); }

/**
 * Reset the string builder so it contains an empty string.
 */
static inline void sb_reset(string_builder_t *sb) {
  if (sb->count > 0) {
    sb->items[0] = 0;
  }
  sb->count = 0;
}

/**
 * Gets a view into the string builder string.
 */
string_view_t sb_view(const string_builder_t *sb);

/**
 * Gets an unmanaged NUL-terminated c-string containing the contents of the
 * builder.
 */
char *sb_cstr(const string_builder_t *sb);

/**
 * Sets a view of a string builder into the actual string builder contents by
 * erasing all characters not part in the view. Note that the provided view
 * should be a subrange of the builder (I.e extraced using sb_view()).
 *
 * This function allows for string view operations to easily carry over to
 * string builders, simply use sb_view() to extract the builder view, do some
 * work, can call sb_canonicalize_view() to persist the changes.
 *
 * Example:
 *    string_view_t view = sb_view(sb);
 *    view = sv_trim(view, " ");
 *    view = sb_canonicalize_view(sb, view);
 *
 * @param sb String builder to shrink to fit the given view.
 * @param view View into sb to canonicalize.
 *
 * @return The new shifted view.
 */
string_view_t sb_canonicalize_view(string_builder_t *sb, string_view_t view);

/**
 * Appends characters to the end of the string builder.
 *
 * @param sb Pointer to the string builder to append to.
 * @param item Item to append.
 *
 * @return A pointer to the string builder string, or NULL if an allocation
 *         failure occured.
 * @{
 */
char *sb_append_char(string_builder_t *sb, char item);
char *sb_append_cstr(string_builder_t *sb, const char *item);
char *sb_append_sv(string_builder_t *sb, string_view_t item);
/**
 * @}
 */

/**
 * Append a string and format it as an ASCII C string literal.
 *
 * @param sb String builder to append to.
 * @param item String containing the literal to append.
 *
 * @return A pointer to the string builder string, or NULL if an allocation
 *         failure occured.
 */
char *sb_append_string_literal(string_builder_t *sb, string_view_t item);

/**
 * Joins strings with an intermediate delimiter.
 *
 * @param sb String builder to append the result to.
 * @param delim Delimiter to place between each item.
 * @param count Number of items to join.
 * @param items Items to join.
 *
 * @return A pointer to the string builder string, or NULL if an allocation
 *         failure occured.
 * @{
 */
char *sb_join_sv(string_builder_t *sb, string_view_t delim, size_t count,
                 const string_view_t *items);
char *sb_join_cstr(string_builder_t *sb, string_view_t delim, size_t count,
                   const char **items);
char *sb_join_sb(string_builder_t *sb, string_view_t delimt, size_t count,
                 const string_builder_t *items);
/**
 * @}
 */

/**
 * Append formatted output to a string builder.
 *
 * @param sb String builder to append to.
 * @param fmt Format string.
 * @param va Format parameters.
 *
 * @return A pointer to the string builder string, or NULL if an allocation
 *         failure occured.
 * @{
 */
char *sb_format(string_builder_t *sb, const char *fmt, ...);
char *sb_vformat(string_builder_t *sb, const char *fmt, va_list va);
/**
 * @}
 */

/**
 * Read characters from a file stream.
 *
 * @param sb String builder to append the contents of the file to.
 * @param f File stream to get characters from.
 * @param max_read Maximum number of characters to read (use SIZE_MAX to read
 *                 the entire file).
 *
 * @return A pointer to the string builder string, or NULL if an allocation
 *         failure occured.
 */
char *sb_read_file(string_builder_t *sb, FILE *f, size_t max_read);

/**
 * Read the next line from a file. This function will consume the newline
 * delimiter from the stream, but will not append it to the builder.
 *
 * @param sb String builder to append the read line to.
 * @param f File to read characters from.
 *
 * @return A pointer to the string builder string, or NULL if an allocation
 *         failure occured.
 */
char *sb_read_line(string_builder_t *sb, FILE *f);

/**
 * Read characters from a file into a string builder until a specified delimiter
 * is encountered. The delimiter is not appended to the string builder, and is
 * consumed from the stream.
 *
 * @param sb String builder to append to.
 * @param f File to read from.
 * @param delim Delimiter to terminate at.
 *
 * @return A pointer to the string builder string, or NULL if an allocation
 *         failure occured.
 */
char *sb_read_until(string_builder_t *sb, FILE *f, string_view_t delim);

#define STRING_BUILDER_H_
#endif
