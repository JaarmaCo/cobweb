#ifndef JSON_H_

#include <stdint.h>

#include "allocator.h"
#include "string_builder.h"
#include "string_view.h"

typedef struct hashmap_sv_json json_object_t;
typedef struct dynamic_array_json json_array_t;
typedef string_builder_t json_string_t;
typedef long double json_number_t;
typedef uint32_t json_id_t;
typedef struct json_pool json_pool_t;
typedef struct json_node json_node_t;

typedef enum json_type {
  JSON_NULL,
  JSON_TRUE,
  JSON_FALSE,
  JSON_NUMBER,
  JSON_STRING,
  JSON_ARRAY,
  JSON_OBJECT,
} json_type_t;

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

#define JSON_H_
#endif
