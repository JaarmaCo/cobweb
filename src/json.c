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

#include "json.h"
#include "hash.h"

#define TYPE_0 json_array_t
#define TYPE_1 json_node_t
#define PREFIX json_array_
#define C_SOURCE
#include "dynamic_array.h"

#define TYPE_1 char *
#define PREFIX da_
#define SUFFIX _str
#define HEADER_ONLY
#include "dynamic_array.h"

#define TYPE_1 size_t
#define PREFIX da_
#define SUFFIX _uz
#define HEADER_ONLY
#include "dynamic_array.h"

#define TYPE_1 long double
#define PREFIX da_
#define SUFFIX _ld
#define HEADER_ONLY
#include "dynamic_array.h"

#define TYPE_1 json_node_t *
#define PREFIX da_
#define SUFFIX _jp
#define HEADER_ONLY
#include "dynamic_array.h"

#define TYPE_1 json_object_entry_t *
#define PREFIX da_
#define SUFFIX _ep
#define HEADER_ONLY
#include "dynamic_array.h"

#define TYPE_0 json_object_t
#define TYPE_1 json_object_entry_t
#define TYPE_2 string_view_t
#define TYPE_3 json_node_t
#define FUNCTION_1 sv_hash
#define FUNCTION_2 sv_equals
#define C_SOURCE
#define PREFIX json_object_
#include "hashmap.h"

void json_destroy_pool(json_pool_t *pool) {

  // Free strings
  //
  for (size_t i = 0; i < pool->string_count; ++i) {
    allocator_release(pool->allocator, pool->string_items[i],
                      pool->string_capacities[i] * sizeof(char),
                      _Alignof(char));
  }
  allocator_release(pool->allocator, pool->string_items,
                    pool->string_capacity * sizeof(char *), _Alignof(char *));
  allocator_release(pool->allocator, pool->string_counts,
                    pool->string_capacity * sizeof(size_t), _Alignof(size_t));
  allocator_release(pool->allocator, pool->string_capacities,
                    pool->string_capacity * sizeof(size_t), _Alignof(size_t));
  pool->string_count = 0;
  pool->string_capacity = 0;
  pool->string_items = NULL;
  pool->string_counts = NULL;
  pool->string_capacities = NULL;

  // Free arrays
  //
  for (size_t i = 0; i < pool->array_count; ++i) {
    allocator_release(pool->allocator, pool->array_items[i],
                      pool->array_capacities[i] * sizeof(json_node_t),
                      _Alignof(json_node_t));
  }
  allocator_release(pool->allocator, pool->array_items,
                    pool->array_capacity * sizeof(json_node_t *),
                    _Alignof(json_node_t *));
  allocator_release(pool->allocator, pool->array_counts,
                    pool->array_capacity * sizeof(size_t), _Alignof(size_t));
  allocator_release(pool->allocator, pool->array_capacities,
                    pool->array_capacity * sizeof(size_t), _Alignof(size_t));
  pool->array_count = 0;
  pool->array_capacity = 0;
  pool->array_items = NULL;
  pool->array_counts = NULL;
  pool->array_capacities = NULL;

  // Free objects
  //
  for (size_t i = 0; i < pool->object_count; ++i) {
    allocator_release(pool->allocator, pool->object_items[i],
                      pool->object_capacities[i] * sizeof(json_object_entry_t),
                      _Alignof(json_object_entry_t));
  }
  allocator_release(pool->allocator, pool->object_items,
                    pool->object_capacity * sizeof(json_object_entry_t *),
                    _Alignof(json_object_entry_t *));
  allocator_release(pool->allocator, pool->object_counts,
                    pool->object_capacity * sizeof(size_t), _Alignof(size_t));
  allocator_release(pool->allocator, pool->object_capacities,
                    pool->object_capacity * sizeof(size_t), _Alignof(size_t));
  pool->object_count = 0;
  pool->object_capacity = 0;
  pool->object_items = NULL;
  pool->object_counts = NULL;
  pool->object_capacities = NULL;

  // Free numbers
  //
  allocator_release(pool->allocator, pool->numbers,
                    pool->number_capacity * sizeof(json_number_t),
                    _Alignof(json_number_t));
  pool->number_count = 0;
  pool->number_capacity = 0;
  pool->numbers = NULL;
}

_Noreturn static void handle_error(json_pool_t *pool) {
  if (pool->on_allocation_failed) {
    pool->on_allocation_failed(pool->allocator);
  }
  exit(1);
}

void json_update_node(json_node_t node, ...) {

  if (node.node_id == 0) {
    return;
  }

  va_list va;
  va_start(va, node);
  switch (node.type) {
  case JSON_OBJECT:
    json_update_object_node(node, va_arg(va, json_object_t));
    break;
  case JSON_ARRAY:
    json_update_array_node(node, va_arg(va, json_array_t));
    break;
  case JSON_STRING:
    json_update_string_node(node, va_arg(va, json_string_t));
    break;
  default:
    break;
  }
  va_end(va);
}

void json_update_object_node(json_node_t node, json_object_t va) {

  if (node.node_id == 0) {
    return;
  }

  node.pool->object_items[node.node_id - 1] = va.items;
  node.pool->object_counts[node.node_id - 1] = va.count;
  node.pool->object_capacities[node.node_id - 1] = va.capacity;
}

void json_update_array_node(json_node_t node, json_array_t va) {

  if (node.node_id == 0) {
    return;
  }

  node.pool->array_items[node.node_id - 1] = va.items;
  node.pool->array_counts[node.node_id - 1] = va.count;
  node.pool->array_capacities[node.node_id - 1] = va.capacity;
}

void json_update_string_node(json_node_t node, json_string_t va) {

  if (node.node_id == 0) {
    return;
  }

  node.pool->string_items[node.node_id - 1] = va.items;
  node.pool->string_counts[node.node_id - 1] = va.count;
  node.pool->string_capacities[node.node_id - 1] = va.capacity;
}

json_node_t json_new_number(json_pool_t *pool, json_number_t value) {
  dynamic_array_ld da = {
      .allocator = pool->allocator,
      .items = pool->numbers,
      .count = pool->number_count,
      .capacity = pool->number_capacity,
  };

  if (!da_append_ld(&da, value)) {
    handle_error(pool);
  }

  pool->numbers = da.items;
  pool->number_count = da.count;
  pool->number_capacity = da.capacity;

  return (json_node_t){
      .type = JSON_NUMBER,
      .node_id = da.count,
      .pool = pool,
  };
}

json_node_t json_new_array(json_pool_t *pool) {

  dynamic_array_jp items = {
      .allocator = pool->allocator,
      .items = pool->array_items,
      .count = pool->array_count,
      .capacity = pool->array_capacity,
  };

  dynamic_array_uz counts = {
      .allocator = pool->allocator,
      .items = pool->array_counts,
      .count = pool->array_count,
      .capacity = pool->array_capacity,
  };

  dynamic_array_uz capacities = {
      .allocator = pool->allocator,
      .items = pool->array_capacities,
      .count = pool->array_count,
      .capacity = pool->array_capacity,
  };

  if (!da_append_jp(&items, 0)) {
    handle_error(pool);
  }

  if (!da_append_uz(&counts, 0)) {
    handle_error(pool);
  }

  if (!da_append_uz(&capacities, 0)) {
    handle_error(pool);
  }

  // NOTE: Since the arrays use the same underlying implementation, the
  // capacities should always be equal.
  assert(items.capacity == counts.capacity &&
         items.capacity == capacities.capacity);
  pool->array_items = (json_node_t **)items.items;
  pool->array_counts = counts.items;
  pool->array_capacities = capacities.items;

  pool->array_count = items.count;
  pool->array_capacity = items.capacity;

  return (json_node_t){
      .type = JSON_ARRAY,
      .node_id = pool->array_count,
      .pool = pool,
  };
}

json_node_t json_new_object(json_pool_t *pool) {

  dynamic_array_ep items = {
      .allocator = pool->allocator,
      .items = pool->object_items,
      .count = pool->object_count,
      .capacity = pool->object_capacity,
  };

  dynamic_array_uz counts = {
      .allocator = pool->allocator,
      .items = pool->object_counts,
      .count = pool->object_count,
      .capacity = pool->object_capacity,
  };

  dynamic_array_uz capacities = {
      .allocator = pool->allocator,
      .items = pool->object_capacities,
      .count = pool->object_count,
      .capacity = pool->object_capacity,
  };

  if (!da_append_ep(&items, 0)) {
    handle_error(pool);
  }

  if (!da_append_uz(&counts, 0)) {
    handle_error(pool);
  }

  if (!da_append_uz(&capacities, 0)) {
    handle_error(pool);
  }

  // NOTE: Since the objects use the same underlying implementation, the
  // capacities should always be equal.
  assert(items.capacity == counts.capacity &&
         items.capacity == capacities.capacity);
  pool->object_items = items.items;
  pool->object_counts = counts.items;
  pool->object_capacities = capacities.items;

  pool->object_count = items.count;
  pool->object_capacity = items.capacity;

  return (json_node_t){
      .type = JSON_OBJECT,
      .node_id = pool->object_count,
      .pool = pool,
  };
}

json_node_t json_new_string(json_pool_t *pool, json_string_t sb) {

  dynamic_array_str items = {
      .allocator = pool->allocator,
      .items = pool->string_items,
      .count = pool->string_count,
      .capacity = pool->string_capacity,
  };

  dynamic_array_uz counts = {
      .allocator = pool->allocator,
      .items = pool->string_counts,
      .count = pool->string_count,
      .capacity = pool->string_capacity,
  };

  dynamic_array_uz capacities = {
      .allocator = pool->allocator,
      .items = pool->string_capacities,
      .count = pool->string_count,
      .capacity = pool->string_capacity,
  };

  if (!da_append_str(&items, sb.items)) {
    handle_error(pool);
  }

  if (!da_append_uz(&counts, sb.count)) {
    handle_error(pool);
  }

  if (!da_append_uz(&capacities, sb.capacity)) {
    handle_error(pool);
  }

  // NOTE: Since the strings use the same underlying implementation, the
  // capacities should always be equal.
  assert(items.capacity == counts.capacity &&
         items.capacity == capacities.capacity);
  pool->string_items = (char **)items.items;
  pool->string_counts = counts.items;
  pool->string_capacities = capacities.items;

  pool->string_count = items.count;
  pool->string_capacity = items.capacity;

  return (json_node_t){
      .type = JSON_STRING,
      .node_id = pool->string_count,
      .pool = pool,
  };
}

json_node_t json_new(json_pool_t *pool, json_type_t type, ...) {
  va_list va;
  va_start(va, type);

  json_node_t result;
  switch (type) {
  case JSON_NULL:
  case JSON_TRUE:
  case JSON_FALSE:
    result = (json_node_t){.type = type, .node_id = 1, .pool = pool};
    break;
  case JSON_NUMBER:
    result = json_new_number(pool, va_arg(va, json_number_t));
    break;
  case JSON_STRING:
    result = json_new_string(pool, va_arg(va, json_string_t));
    break;
  case JSON_ARRAY:
    result = json_new_array(pool);
    break;
  case JSON_OBJECT:
    result = json_new_object(pool);
    break;
  }

  va_end(va);
  return result;
}

size_t json_count(json_node_t node) {
  if (node.node_id == 0) {
    return 0;
  }
  switch (node.type) {
  case JSON_ARRAY:
    return node.pool->array_counts[node.node_id - 1];
  case JSON_OBJECT:
    return node.pool->object_counts[node.node_id - 1];
  default:
    return 1;
  }
}

bool json_iterate_properties(json_node_t node, size_t *state,
                             string_view_t *out_key, json_node_t *out_value) {

  if (node.node_id == 0 || node.type != JSON_OBJECT) {
    return false;
  }

  size_t capacity = node.pool->object_capacities[node.node_id - 1];
  json_object_entry_t *items = node.pool->object_items[node.node_id - 1];

  size_t i;
  json_object_entry_t *entry = NULL;
  for (i = *state; i < capacity; ++i) {
    entry = &items[i];
    if (entry->hash) {
      break;
    } else {
      entry = NULL;
    }
  }

  if (NULL == entry) {
    return false;
  }

  if (out_key) {
    *out_key = entry->key;
  }

  if (out_value) {
    *out_value = entry->value;
  }

  *state = i + 1;
  return true;
}

bool json_get(json_node_t node, json_type_t type, ...) {

  if (node.node_id == 0) {
    return false;
  }

  va_list va;
  va_start(va, type);

  bool match = false;
  switch (type) {
  case JSON_TRUE:
  case JSON_FALSE:
  case JSON_NULL:
    match = type == node.type;
    break;
  case JSON_NUMBER:
    if (node.type == JSON_NUMBER) {
      *va_arg(va, json_number_t *) = json_as_number(node);
      match = true;
    }
    break;
  case JSON_STRING:
    if (node.type == JSON_STRING) {
      *va_arg(va, json_string_t *) = json_as_string(node);
      match = true;
    }
    break;
  case JSON_ARRAY:
    if (node.type == JSON_ARRAY) {
      *va_arg(va, json_array_t *) = json_as_array(node);
      match = true;
    }
    break;
  case JSON_OBJECT:
    if (node.type == JSON_OBJECT) {
      *va_arg(va, json_object_t *) = json_as_object(node);
      match = true;
    }
    break;
  }

  va_end(va);
  return match;
}

static void persist_string(json_pool_t *pool, string_view_t *string) {
  json_string_t builder = {
      .allocator = pool->allocator,
  };

  if (!sb_append_sv(&builder, *string)) {
    handle_error(pool);
  }

  if (json_new_string(pool, builder).node_id == 0) {
    handle_error(pool);
  }

  *string = sb_view(&builder);
}

bool json_insert(json_node_t node, string_view_t key, json_node_t new_node) {

  json_object_t object;
  if (!json_get(node, JSON_OBJECT, &object)) {
    return false;
  }

  if (json_object_find(&object, key)) {
    return false;
  }

  persist_string(node.pool, &key);

  if (json_object_insert(&object, key, new_node)) {
    json_update_object_node(node, object);
    return true;
  }
  return false;
}

bool json_replace(json_node_t node, string_view_t key, json_node_t new_node) {

  json_object_t object;
  if (!json_get(node, JSON_OBJECT, &object)) {
    return false;
  }

  json_object_entry_t *entry = json_object_find(&object, key);
  if (entry) {
    entry->value = new_node;
    return true;
  }
  return false;
}

void json_put(json_node_t node, string_view_t key, json_node_t new_node) {

  json_object_t object;
  if (!json_get(node, JSON_OBJECT, &object)) {
    return;
  }

  json_object_entry_t *entry = json_object_find(&object, key);
  if (entry) {
    entry->value = new_node;
    return;
  }

  persist_string(node.pool, &key);

  if (!json_object_insert(&object, key, new_node)) {
    handle_error(node.pool);
  }
  json_update_object_node(node, object);
}

json_node_t *json_find(json_node_t node, string_view_t key) {

  json_object_t object;
  if (!json_get(node, JSON_OBJECT, &object)) {
    return NULL;
  }

  json_object_entry_t *entry = json_object_find(&object, key);
  return entry ? &entry->value : NULL;
}

json_node_t *json_at(json_node_t node, size_t index) {

  json_array_t array;
  if (!json_get(node, JSON_ARRAY, &array)) {
    return NULL;
  }
  return index < array.count ? &array.items[index] : NULL;
}

void json_remove(json_node_t node, json_node_t *entry) {

  if (node.type == JSON_OBJECT) {

    json_object_t object;
    if (!json_get(node, JSON_OBJECT, &object)) {
      return;
    }

    json_object_entry_t *real_entry =
        (json_object_entry_t *)((intptr_t)entry -
                                offsetof(json_object_entry_t, value));
    json_object_remove(&object, real_entry);
    json_update_object_node(node, object);
  } else if (node.type == JSON_ARRAY) {

    json_array_t array;
    if (!json_get(node, JSON_ARRAY, &array)) {
      return;
    }

    ptrdiff_t offset = entry - array.items;
    if (offset < 0 || offset > (ptrdiff_t)array.count) {
      return;
    }
    json_array_remove(&array, (size_t)offset);
    json_update_array_node(node, array);
  }
}

bool json_delete(json_node_t node, string_view_t key) {

  json_object_t object;
  if (!json_get(node, JSON_OBJECT, &object)) {
    return false;
  }

  if (json_object_delete(&object, key)) {
    json_update_object_node(node, object);
    return true;
  }
  return false;
}

void json_append(json_node_t node, json_node_t new_node) {

  json_array_t array;
  if (!json_get(node, JSON_ARRAY, &array)) {
    return;
  }

  if (!json_array_append(&array, new_node)) {
    handle_error(node.pool);
  }
  json_update_array_node(node, array);
}

void json_append_range(json_node_t node, size_t count,
                       const json_node_t *items) {

  json_array_t array;
  if (!json_get(node, JSON_ARRAY, &array)) {
    return;
  }

  if (!json_array_append_range(&array, count, items)) {
    handle_error(node.pool);
  }
  json_update_array_node(node, array);
}

void json_remove_back(json_node_t node) {

  json_array_t array;
  if (!json_get(node, JSON_ARRAY, &array)) {
    return;
  }

  json_array_remove_back(&array);
  json_update_array_node(node, array);
}

void json_remove_back_range(json_node_t node, size_t count) {

  json_array_t array;
  if (!json_get(node, JSON_ARRAY, &array)) {
    return;
  }

  json_array_remove_back_range(&array, count);
  json_update_array_node(node, array);
}

char *json_dumps(json_node_t node, string_builder_t *sb) {

  size_t old_count = sb->count;
  if (node.node_id == 0) {
    if (!sb_append_sv(sb, SV("null"))) {
      return NULL;
    }
    return sb->items + old_count;
  }

  switch (node.type) {
  case JSON_TRUE:
    if (!sb_append_sv(sb, SV("true"))) {
      return NULL;
    }
    break;
  case JSON_FALSE:
    if (!sb_append_sv(sb, SV("false"))) {
      return NULL;
    }
    break;
  case JSON_NULL:
    if (!sb_append_sv(sb, SV("null"))) {
      return NULL;
    }
    break;
  case JSON_NUMBER: {
    json_number_t number;
    json_get(node, JSON_NUMBER, &number);
    if (!sb_format(sb, "%Lg", number)) {
      return NULL;
    }
    break;
  }
  case JSON_STRING: {
    json_string_t string;
    json_get(node, JSON_STRING, &string);
    if (!sb_append_string_literal(sb, sb_view(&string))) {
      return NULL;
    }
    break;
  }
  case JSON_ARRAY: {
    json_array_t array;
    json_get(node, JSON_ARRAY, &array);

    if (!sb_append_char(sb, '[')) {
      return NULL;
    }

    for (size_t i = 0; i < array.count; ++i) {
      if (i != 0) {
        if (!sb_append_char(sb, ',')) {
          return NULL;
        }
      }
      if (!json_dumps(array.items[i], sb)) {
        return NULL;
      }
    }

    if (!sb_append_char(sb, ']')) {
      return NULL;
    }
    break;
  }
  case JSON_OBJECT: {

    if (!sb_append_char(sb, '{')) {
      return NULL;
    }

    size_t i = 0;
    size_t state = 0;
    string_view_t key;
    json_node_t value;
    while (json_iterate_properties(node, &state, &key, &value)) {
      if (i != 0) {
        if (!sb_append_char(sb, ',')) {
          return NULL;
        }
      }

      if (!sb_append_string_literal(sb, key)) {
        return NULL;
      }
      if (!sb_append_char(sb, ':')) {
        return NULL;
      }
      if (!json_dumps(value, sb)) {
        return NULL;
      }
      ++i;
    }

    if (!sb_append_char(sb, '}')) {
      return NULL;
    }
    break;
  }
  }
  return sb->items + old_count;
}

void json_dumpf(json_node_t node, FILE *f) {
  // TODO: This can be made more efficient by removing the intermediate string
  // builder.
  string_builder_t sb = {
      .allocator = node.pool->allocator,
  };
  json_dumps(node, &sb);
  fputs(sb_cstr(&sb), f);
  sb_destroy(&sb);
}

bool json_equals(json_node_t lhs, json_node_t rhs) {

  if (lhs.node_id == 0) {
    return rhs.node_id == 0 || rhs.type == JSON_NULL;
  }

  if (rhs.node_id == 0) {
    return lhs.type == JSON_NULL;
  }

  if (lhs.type != rhs.type) {
    return false;
  }

  if (lhs.node_id == rhs.node_id) {
    return true;
  }

  string_view_t key = {0};
  json_node_t value = {0};

  switch (lhs.type) {
  case JSON_FALSE:
  case JSON_TRUE:
  case JSON_NULL:
    return true;
  case JSON_NUMBER: {
    json_number_t x = 0, y = 0;
    json_get(lhs, JSON_NUMBER, &x);
    json_get(rhs, JSON_NUMBER, &y);
    return x == y;
  }
  case JSON_STRING: {
    json_string_t x = {0}, y = {0};
    json_get(lhs, JSON_STRING, &x);
    json_get(rhs, JSON_STRING, &y);
    return sv_equals(sb_view(&x), sb_view(&y));
  }
  case JSON_ARRAY:
    if (json_count(lhs) != json_count(rhs)) {
      return false;
    }
    for (size_t i = 0; i < json_count(lhs); ++i) {
      if (!json_equals(*json_at(lhs, i), *json_at(rhs, i))) {
        return false;
      }
    }
    return true;
  case JSON_OBJECT:
    if (json_count(lhs) != json_count(rhs)) {
      return false;
    }
    for (size_t state = 0;
         json_iterate_properties(lhs, &state, &key, &value);) {
      json_node_t *other_value = json_find(rhs, key);
      if (!other_value || !json_equals(value, *other_value)) {
        return false;
      }
    }
  }
  return true;
}

size_t json_hash(json_node_t x) {
  if (x.node_id == 0 || x.type == JSON_NULL) {
    return 0;
  }
  switch (x.type) {
  case JSON_TRUE:
  case JSON_FALSE:
  case JSON_NULL:
    return hash_int((int)x.type);
  case JSON_STRING: {
    json_string_t string = {0};
    json_get(x, JSON_STRING, &string);
    return sv_hash(sb_view(&string));
  }
  case JSON_NUMBER: {
    json_number_t number = 0;
    json_get(x, JSON_NUMBER, &number);
    return hash_long_double(number);
  }
  case JSON_ARRAY: {
    size_t hash = (size_t)JSON_ARRAY;
    for (size_t i = 0; i < json_count(x); ++i) {
      hash = hash_combine(hash, json_hash(*json_at(x, i)));
    }
    return hash;
  }
  case JSON_OBJECT: {
    size_t hash = (size_t)JSON_OBJECT;
    string_view_t key = {0};
    json_node_t value = {0};
    for (size_t state = 0; json_iterate_properties(x, &state, &key, &value);) {
      size_t entry_hash = hash_combine(sv_hash(key), json_hash(value));
      hash = hash_combine(hash, entry_hash);
    }
    return hash;
  }
  }

  fprintf(stderr, "Unreachable");
  abort();
}

bool json_as_boolean(json_node_t node) { return node.type == JSON_TRUE; }

json_number_t json_as_number(json_node_t node) {
  return node.pool->numbers[node.node_id - 1];
}

json_string_t json_as_string(json_node_t node) {
  return (json_string_t){
      .allocator = node.pool->allocator,
      .items = node.pool->string_items[node.node_id - 1],
      .count = node.pool->string_counts[node.node_id - 1],
      .capacity = node.pool->string_capacities[node.node_id - 1],
  };
}

json_array_t json_as_array(json_node_t node) {
  return (json_array_t){
      .allocator = node.pool->allocator,
      .items = node.pool->array_items[node.node_id - 1],
      .count = node.pool->array_counts[node.node_id - 1],
      .capacity = node.pool->array_capacities[node.node_id - 1],
  };
}

json_object_t json_as_object(json_node_t node) {
  return (json_object_t){
      .allocator = node.pool->allocator,
      .items = (void *)node.pool->object_items[node.node_id - 1],
      .count = node.pool->object_counts[node.node_id - 1],
      .capacity = node.pool->object_capacities[node.node_id - 1],
  };
}
