#include "json.h"
#include "dynamic_array_intptr_t.h"
#include "dynamic_array_json_node_t.h"
#include "dynamic_array_long_double.h"
#include "dynamic_array_size_t.h"
#include "hashmap_sv_json.h"

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
                      pool->object_capacities[i] *
                          sizeof(hashmap_entry_sv_json),
                      _Alignof(hashmap_entry_sv_json));
  }
  allocator_release(pool->allocator, pool->object_items,
                    pool->object_capacity * sizeof(hashmap_entry_sv_json *),
                    _Alignof(hashmap_entry_sv_json *));
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

  dynamic_array_iptr items = {
      .allocator = pool->allocator,
      .items = (intptr_t *)pool->array_items,
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

  if (!da_append_iptr(&items, 0)) {
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

  dynamic_array_iptr items = {
      .allocator = pool->allocator,
      .items = (intptr_t *)pool->object_items,
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

  if (!da_append_iptr(&items, 0)) {
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
  pool->object_items = (void **)items.items;
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

  dynamic_array_iptr items = {
      .allocator = pool->allocator,
      .items = (intptr_t *)pool->string_items,
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

  if (!da_append_iptr(&items, (intptr_t)sb.items)) {
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
  hashmap_entry_sv_json *items = node.pool->object_items[node.node_id - 1];

  size_t i;
  hashmap_entry_sv_json *entry = NULL;
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
      *va_arg(va, json_number_t *) = node.pool->numbers[node.node_id - 1];
      match = true;
    }
    break;
  case JSON_STRING:
    if (node.type == JSON_STRING) {
      json_string_t *out = va_arg(va, json_string_t *);
      out->allocator = node.pool->allocator;
      out->items = node.pool->string_items[node.node_id - 1];
      out->count = node.pool->string_counts[node.node_id - 1];
      out->capacity = node.pool->string_capacities[node.node_id - 1];
      match = true;
    }
    break;
  case JSON_ARRAY:
    if (node.type == JSON_ARRAY) {
      json_array_t *out = va_arg(va, json_array_t *);
      out->allocator = node.pool->allocator;
      out->items = node.pool->array_items[node.node_id - 1];
      out->count = node.pool->array_counts[node.node_id - 1];
      out->capacity = node.pool->array_capacities[node.node_id - 1];
      match = true;
    }
    break;
  case JSON_OBJECT:
    if (node.type == JSON_OBJECT) {
      json_object_t *out = va_arg(va, json_object_t *);
      out->allocator = node.pool->allocator;
      out->items = node.pool->object_items[node.node_id - 1];
      out->count = node.pool->object_counts[node.node_id - 1];
      out->capacity = node.pool->object_capacities[node.node_id - 1];
      match = true;
    }
    break;
  }

  va_end(va);
  return match;
}

bool json_insert(json_node_t node, string_view_t key, json_node_t new_node) {

  json_object_t object;
  if (!json_get(node, JSON_OBJECT, &object)) {
    return false;
  }

  if (hm_insert_sv_json(&object, key, new_node)) {
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

  if (hm_replace_sv_json(&object, key, new_node)) {
    json_update_object_node(node, object);
    return true;
  }
  return false;
}

void json_put(json_node_t node, string_view_t key, json_node_t new_node) {

  json_object_t object;
  if (!json_get(node, JSON_OBJECT, &object)) {
    return;
  }

  if (!hm_put_sv_json(&object, key, new_node)) {
    handle_error(node.pool);
  }
  json_update_object_node(node, object);
}

json_node_t *json_find(json_node_t node, string_view_t key) {

  json_object_t object;
  if (!json_get(node, JSON_OBJECT, &object)) {
    return NULL;
  }

  hashmap_entry_sv_json *entry = hm_find_sv_json(&object, key);
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

    hashmap_entry_sv_json *real_entry =
        (hashmap_entry_sv_json *)((intptr_t)entry -
                                  offsetof(hashmap_entry_sv_json, value));
    hm_remove_sv_json(&object, real_entry);
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
    da_remove_json(&array, (size_t)offset);
    json_update_array_node(node, array);
  }
}

bool json_delete(json_node_t node, string_view_t key) {

  json_object_t object;
  if (!json_get(node, JSON_OBJECT, &object)) {
    return false;
  }

  if (hm_delete_sv_json(&object, key)) {
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

  if (!da_append_json(&array, new_node)) {
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

  if (!da_append_range_json(&array, count, items)) {
    handle_error(node.pool);
  }
  json_update_array_node(node, array);
}

void json_remove_back(json_node_t node) {

  json_array_t array;
  if (!json_get(node, JSON_ARRAY, &array)) {
    return;
  }

  da_remove_back_json(&array);
  json_update_array_node(node, array);
}

void json_remove_back_range(json_node_t node, size_t count) {

  json_array_t array;
  if (!json_get(node, JSON_ARRAY, &array)) {
    return;
  }

  da_remove_back_range_json(&array, count);
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
