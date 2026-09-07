#ifndef JSON_H_

#include <stdint.h>

#include "allocator.h"
#include "string_builder.h"
#include "string_view.h"

#define JSON_ERROR_UNRECOGNIZED_TOKEN 1
#define JSON_ERROR_UNEXPECTED_EOF 2
#define JSON_ERROR_INVALID_START_OF_STRING 3
#define JSON_ERROR_INVALID_START_OF_ARRAY 4
#define JSON_ERROR_INVALID_START_OF_OBJECT 5
#define JSON_ERROR_INVALID_ESCAPE_SEQUENCE 6
#define JSON_WARNING_UNICODE_ESCAPE_SEQUENCES_NOT_SUPPORTED 100
#define JSON_ERROR_MEMORY_ALLOCATION_FAILED -99

typedef struct hashmap_sv_json json_object_t;
typedef struct dynamic_array_json json_array_t;
typedef string_builder_t json_string_t;
typedef long double json_number_t;
typedef uint32_t json_id_t;
typedef struct json_pool json_pool_t;
typedef struct json_node json_node_t;
typedef struct json_parser json_parser_t;

typedef enum json_type {
  JSON_NULL,
  JSON_TRUE,
  JSON_FALSE,
  JSON_NUMBER,
  JSON_STRING,
  JSON_ARRAY,
  JSON_OBJECT,
} json_type_t;

/**
 * Enumerates the different parser events that can occur when parsing a JSON
 * document.
 */
typedef enum json_parser_event {

  /**
   * A new scalar element was read into the parser.
   *
   * If the scalar is a string, or number then it's value is present in the
   * parser scratch buffer.
   */
  JSON_EVENT_SCALAR,

  /**
   * Encountered the start of an object (I.e '{')
   */
  JSON_EVENT_OBJECT_BEGIN,

  /**
   * Encountered an object attribute key.
   */
  JSON_EVENT_OBJECT_KEY,

  /**
   * Encountered the end of an object (I.e '}')
   */
  JSON_EVENT_OBJECT_END,

  /**
   * Encountered the start of an array (I.e '[')
   */
  JSON_EVENT_ARRAY_BEGIN,

  /**
   * Encountered the end of an array (I.e ']')
   */
  JSON_EVENT_ARRAY_END,

} json_parser_event_t;

struct json_node {

  /**
   * Specifies the JSON type of this node.
   */
  json_type_t type;

  /**
   * Id used to retrieve the node value.
   */
  json_id_t node_id;

  /**
   * Pointer to the pool this node belongs to.
   */
  json_pool_t *pool;
};

/**
 * Pool for storing json values.
 */
struct json_pool {

  /**
   * Allocator to use.
   */
  allocator_t *allocator;

  /**
   * Callback that handles allocation errors, or NULL to call exit() on an
   * error.
   */
  void (*on_allocation_failed)(allocator_t *);

  /**
   * JSON key and string storage.
   * @{
   */
  size_t string_count;
  size_t string_capacity;
  char **string_items;
  size_t *string_counts;
  size_t *string_capacities;
  /**
   * @}
   */

  /**
   * JSON number storage.
   * @{
   */
  size_t number_count;
  size_t number_capacity;
  json_number_t *numbers;

  /**
   * JSON array storage.
   * @{
   */
  size_t array_count;
  size_t array_capacity;
  json_node_t **array_items;
  size_t *array_counts;
  size_t *array_capacities;
  /**
   * @}
   */

  /**
   * JSON object storage.
   * @{
   */
  size_t object_count;
  size_t object_capacity;
  void **object_items;
  size_t *object_counts;
  size_t *object_capacities;
  /**
   * @}
   */
};

struct json_parser {

  /**
   * Temporary buffer that contains currently parsed scalar values as a string.
   */
  json_string_t scratch;

  /**
   * Lookahead variable for peeking at the frontmost character.
   */
  int current_char;

  /**
   * Callback for getting the next character (EOF on end of input).
   */
  int (*get_next)(json_parser_t *parser);

  /**
   * Callback that handles parsing errors.
   *
   * @param parser The parser where the error occured.
   * @param error_code The numeric code of the error.
   * @param error_string A human-readable error message.
   */
  void (*on_error)(json_parser_t *parser, int error_code,
                   string_view_t error_string);

  /**
   * Processes a parsing event.
   *
   * @param parser Parser where the event was raised.
   * @param event The parser event.
   * @param type The JSON type of the value being parsed.
   *
   * @return true on success
   */
  bool (*on_event)(json_parser_t *parser, json_parser_event_t event,
                   json_type_t type);

  /**
   * Parser line information.
   */
  struct {

    /**
     * Line number.
     */
    int line;

    /**
     * Column number.
     */
    int column;

    /**
     * File offset in bytes.
     */
    int offset;

  } lineno;

  /**
   * A reserved user-defined arguments that may be used by parser
   * implementations.
   */
  void *user1;
  void *user2;
  void *user3;
  void *user4;
};

/**
 * Parse JSON using a custom parser.
 *
 * @param parser Parser to execute.
 * @return true if the value could be parsed.
 * @{
 */
bool json_parse(json_parser_t *parser);
bool json_parse_keyword(json_parser_t *parser);
bool json_parse_string(json_parser_t *parser);
bool json_parse_number(json_parser_t *parser);
bool json_parse_array(json_parser_t *parser);
bool json_parse_object(json_parser_t *parser);
/**
 * @}
 */

/**
 * Creates a parser that parses JSON values.
 *
 * @param[out] out_parser Pointer to a variable that receives the parser.
 * @param pool Pool used to allocate the JSON values.
 * @param get Callback for getting characters.
 * @param f User defined argument to pass to the get callback.
 *
 * @return true if the parser could be created.
 */
bool json_create_value_parser(json_parser_t *out_parser, json_pool_t *pool,
                              int (*get)(void *), void *f);

/**
 * Gets the result from a value parser.
 */
json_node_t json_value_parser_result(const json_parser_t *parser);

/**
 * Deallocates any memory used by a JSON value parser.
 */
void json_destroy_value_parser(json_parser_t *parser);

/**
 * Loads a JSON value from a file.
 *
 * @param pool JSON pool to allocate the value in.
 * @param f Stream to read from.
 *
 * @return The parsed node, or a node with node_id = 0 on error.
 * @{
 */
json_node_t json_load(json_pool_t *pool, FILE *f);
json_node_t json_loads(json_pool_t *pool, string_view_t *f);
/**
 * @}
 */

/**
 * Frees all memory allocated in the pool.
 */
void json_destroy_pool(json_pool_t *pool);

/**
 * Persist changes to a JSON node by writing the changed value to the object
 * pool. Calling this function is required if the node was a non-trivial node,
 * I.e string, array or object and that node had been modified outside of
 * the json_* functions.
 *
 * @param node An object, array or string node that should be updated.
 * @param va The changed node value to set.
 * @{
 */
void json_update_node(json_node_t node, ...);
void json_update_string_node(json_node_t node, json_string_t va);
void json_update_array_node(json_node_t node, json_array_t va);
void json_update_object_node(json_node_t node, json_object_t va);
/**
 * @}
 */

/**
 * Constructs a new json node of a given type.
 *
 * @param pool Pool where the value will be allocated.
 * @param type Type of the value.
 * @param va Initial value of the new string or number node.
 *
 * @return The new node.
 * @{
 */
json_node_t json_new(json_pool_t *pool, json_type_t type, ...);
json_node_t json_new_number(json_pool_t *pool, json_number_t va);
json_node_t json_new_string(json_pool_t *pool, json_string_t va);
json_node_t json_new_array(json_pool_t *pool);
json_node_t json_new_object(json_pool_t *pool);
/**
 * @}
 */

/**
 * Gets the number of elements in an array or object node.
 *
 * @param node Array or object node.
 *
 * @return The number of JSON elements in the array or object node.
 */
size_t json_count(json_node_t node);

/**
 * Iterates over every property in a json object.
 *
 * @param node Object node to iterate over.
 * @param[inout] state Pointer to the iteration state variable.
 * @param[out] out_key Address to write the key to, or NULL.
 * @param[out] out_value Address to write the value to, or NULL.
 *
 * @return false when there are no more elements to fetch, otherwise true.
 */
bool json_iterate_properties(json_node_t node, size_t *state,
                             string_view_t *out_key, json_node_t *out_value);

/**
 * Extract a value from a JSON node.
 *
 * @param node Node to get a value from.
 * @param type Type of value to get.
 * @param va Pointer to a variable to put the value to, or nothing if the type
 *           is JSON_TRUE, JSON_FALSE or JSON_NULL.
 *
 * @return true if the type of the node equals the input type, and a value was
 *         written out.
 */
bool json_get(json_node_t node, json_type_t type, ...);

/**
 * Unchecked getter for a particular JSON value type.
 * @{
 */
bool json_as_boolean(json_node_t node);
json_number_t json_as_number(json_node_t node);
json_string_t json_as_string(json_node_t node);
json_array_t json_as_array(json_node_t node);
json_object_t json_as_object(json_node_t node);
/**
 * @}
 */

/**
 * Insert a new item into a JSON object.
 *
 * @param node Object node to insert a value into.
 * @param key Key of the new property.
 * @param new_node Value of the new property.
 *
 * @return true if the object was inserted, false if that property already
 *         exists.
 */
bool json_insert(json_node_t node, string_view_t key, json_node_t new_node);

/**
 * Replaces the value of a property with a new value.
 *
 * @param node Object node to replace a value in.
 * @param key Key of the property to replace.
 * @param new_node New value to assign to the property.
 *
 * @return true if the value was replaced, false if the property was not
 *         in the object.
 */
bool json_replace(json_node_t node, string_view_t key, json_node_t new_node);

/**
 * Insert or replace a property in a JSON object.
 *
 * @param node Object node to modify.
 * @param key Key of the property to modify.
 * @param new_node Value to set the property to.
 */
void json_put(json_node_t node, string_view_t key, json_node_t new_node);

/**
 * Find a property in the JSON object.
 *
 * @param node Object node to search.
 * @param key Key to look for.
 *
 * @return A pointer to the located property, or NULL if it was not found.
 */
json_node_t *json_find(json_node_t node, string_view_t key);

/**
 * Gets an element from a JSON array.
 *
 * @param node Array node to get an element from.
 * @param index Index of the element.
 *
 * @return A pointer to the array element.
 */
json_node_t *json_at(json_node_t node, size_t index);

/**
 * Removes a property or element from an object node.
 *
 * @param node Object or array node to modify.
 * @param entry Pointer to the node to remove.
 */
void json_remove(json_node_t node, json_node_t *entry);

/**
 * Finds and removes a property in an object node.
 *
 * @param node Node to delete a property from.
 * @param key Key of the property to delete.
 *
 * @return true if the property was deleted, false if the property did not exist
 *         in the object.
 */
bool json_delete(json_node_t node, string_view_t key);

/**
 * Appends a node to the end of an array node.
 *
 * @param node Array node to append a value to.
 * @param new_node Value to append.
 */
void json_append(json_node_t node, json_node_t new_node);

/**
 * Appends a range of values to the end of an array node.
 *
 * @param node Array node to append values to.
 * @param count Number of items to append.
 * @param items Pointer to an array of items to append.
 */
void json_append_range(json_node_t node, size_t count,
                       const json_node_t *items);

/**
 * Removes the last element of an array node.
 *
 * @param node Array node to modify.
 */
void json_remove_back(json_node_t node);

/**
 * Removes a range of elements from an array node.
 *
 * @param node Array node to modify.
 * @param index Index of the first element to remove.
 * @param count Number of elements to remove.
 */
void json_remove_range(json_node_t node, size_t index, size_t count);

/**
 * Dumps a JSON value to a string builder.
 *
 * @param node JSON value to dump.
 * @param sb Builder to append the recorded value to.
 * @param f File to output the contents to.
 *
 * @return Dumped JSON value as a string, or NULL if a memory allocation failed.
 * @{
 */
char *json_dumps(json_node_t node, string_builder_t *sb);
void json_dumpf(json_node_t node, FILE *f);
/**
 * @}
 */

/**
 * Check whether two nodes are equal.
 */
bool json_equals(json_node_t lhs, json_node_t rhs);

/**
 * Hash a JSON value.
 */
size_t json_hash(json_node_t x);

/**
 * Evaluate a JSON path in the given node.
 *
 * Paths always start with a dollar sign '$' to signify the root element that is
 * to be fetched. Following this, the path can consist of either '.<property>'
 * to extract a property from an object, or `[<index>]` to get a value from an
 * array.
 *
 * The path formatter also supports format arguments where %<n> can be put in
 * place of where an array index or property name should be. The type of the
 * format argument is deduced from it's usage in the format string, where size_t
 * is expected for indices and string_view_t for properties.
 *
 * The format specifiers require a number that specifies where in the parameter
 * list the argument is present. This is done by placing a number after the
 * %-sign.
 *
 * Example:
 *
 *    json_node_t customer
 *      = json_path(node, "$.%1.customers[%2]", SV("registry"), (size_t)2);
 *
 * Any conflicts arising from incorrect usage (an argument being inferred as
 * both a string and index), are asserted in debug builds and coerced nullish
 * values at release builds.
 *
 * @param node Node the path is relative to.
 * @param path A JSON path format string to evaluate.
 * @param va Variadic arguments to format into the path string.
 *
 * @return The node the path points to, or a node with node_id = 0 if the path
 *         was invalid.
 * @{
 */
json_node_t json_path(json_node_t node, const char *path, ...);
json_node_t json_vpath(json_node_t node, const char *path, va_list va);
/**
 * @}
 */

#define JSON_H_
#endif
