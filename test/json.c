#if !defined(UNITY_BUILD)
#include "json.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#define ESC "\x1B"
#define BOLD ESC "[1m"
#define RED ESC "[31m"
#define GREEN ESC "[32m"
#define BLUE ESC "[34m"
#define RST ESC "[0m"

#define ERR(str) BOLD RED "[ERROR]: " RST str "\n"
#define OK(str) BOLD GREEN "[OK]: " RST str "\n"
#define INFO(str) BOLD BLUE str RST "\n"

static void on_allocation_failed(allocator_t *unused) {
  (void)unused;
  fprintf(stderr, ERR("Memory allocation failed."));
  exit(1);
}

static json_node_t check_new_null(json_pool_t *pool) {
  printf(INFO("--- Test check_new_null"));
  return json_new(pool, JSON_NULL);
}

static json_node_t check_new_true(json_pool_t *pool) {
  printf(INFO("--- Test check_new_true"));
  return json_new(pool, JSON_TRUE);
}

static json_node_t check_new_false(json_pool_t *pool) {
  printf(INFO("--- Test check_new_false"));
  return json_new(pool, JSON_FALSE);
}

static json_node_t check_new_string(json_pool_t *pool) {
  printf(INFO("--- Test check_new_string"));
  json_string_t sb = {
      .allocator = pool->allocator,
  };
  sb_append_sv(&sb, SV("Hello, world!"));
  return json_new(pool, JSON_STRING, sb);
}

static json_node_t check_new_number(json_pool_t *pool) {
  printf(INFO("--- Test check_new_number"));
  return json_new(pool, JSON_NUMBER, (json_number_t)42);
}

static json_node_t check_new_array(json_pool_t *pool) {
  printf(INFO("--- Test check_new_array"));
  return json_new(pool, JSON_ARRAY);
}

static json_node_t check_new_object(json_pool_t *pool) {
  printf(INFO("--- Test check_new_object"));
  return json_new(pool, JSON_OBJECT);
}

static json_node_t check_parse_true(json_pool_t *pool) {
  printf(INFO("--- Test check_parse_true"));
  string_view_t contents = SV("true");
  return json_loads(pool, &contents);
}

static json_node_t check_parse_false(json_pool_t *pool) {
  printf(INFO("--- Test check_parse_false"));
  string_view_t contents = SV("false");
  return json_loads(pool, &contents);
}

static json_node_t check_parse_null(json_pool_t *pool) {
  printf(INFO("--- Test check_parse_null"));
  string_view_t contents = SV("null");
  return json_loads(pool, &contents);
}

static json_node_t check_parse_number(json_pool_t *pool) {
  printf(INFO("--- Test check_parse_number"));
  string_view_t contents = SV("3.14");
  return json_loads(pool, &contents);
}

static json_node_t check_parse_string(json_pool_t *pool) {
  printf(INFO("--- Test check_parse_string"));
  string_view_t contents = SV("\"\\\"Hello,\\\\ world!\\\"\"");
  return json_loads(pool, &contents);
}

static json_node_t check_parse_empty_array(json_pool_t *pool) {
  printf(INFO("--- Test check_parse_empty_array"));
  string_view_t contents = SV("[ ]");
  return json_loads(pool, &contents);
}

static json_node_t check_parse_singleton_array(json_pool_t *pool) {
  printf(INFO("--- Test check_parse_singleton_array"));
  string_view_t contents = SV("[ 1 ]");
  return json_loads(pool, &contents);
}

static json_node_t check_parse_array(json_pool_t *pool) {
  printf(INFO("--- Test check_parse_array"));
  string_view_t contents = SV("[ 1, 2, 3, 4 ]");
  return json_loads(pool, &contents);
}

static json_node_t check_parse_empty_object(json_pool_t *pool) {
  printf(INFO("--- Test check_parse_empty_object"));
  string_view_t contents = SV("{}");
  return json_loads(pool, &contents);
}

static json_node_t check_parse_singleton_object(json_pool_t *pool) {
  printf(INFO("--- Test check_parse_singleton_object"));
  string_view_t contents = SV("{ \"Foxy\": true }");
  return json_loads(pool, &contents);
}

static json_node_t check_parse_object(json_pool_t *pool) {
  printf(INFO("--- Test check_parse_object"));
  string_view_t contents = SV("{ \"Cain\": 2, \"Able\": 1, \"Differ\": 3 }");
  return json_loads(pool, &contents);
}

static void run_test(json_node_t (*test)(json_pool_t *pool),
                     string_view_t expected) {

  size_t allocated_size = 0;
  debug_allocator_t a =
      trace_free_balance_allocator(malloc_allocator, &allocated_size);

  json_pool_t pool = {
      .allocator = &a.base,
      .on_allocation_failed = on_allocation_failed,
  };

  json_node_t result = test(&pool);

  string_builder_t sb = {
      .allocator = &a.base,
  };
  json_dumps(result, &sb);

  printf("%.*s\n", (int)sb.count, sb.items);

  string_view_t result_string = sb_view(&sb);
  if (!sv_equals(expected, result_string)) {
    fprintf(stderr,
            ERR("Expected value %.*s did not match the actual value %.*s"),
            (int)expected.count, expected.items, (int)result_string.count,
            result_string.items);
    exit(1);
  }

  printf(OK("Values match."));

  sb_destroy(&sb);
  json_destroy_pool(&pool);

  if (allocated_size != 0) {
    fprintf(stderr, ERR("Leaked %zu bytes"), allocated_size);
    exit(1);
  }
}

json_node_t check_object_construct(json_pool_t *pool) {
  printf(INFO("--- Test check_object_construct"));

  json_node_t object = json_new(pool, JSON_OBJECT);
  json_insert(object, SV("aaa"), json_new(pool, JSON_TRUE));
  json_insert(object, SV("bbb"), json_new(pool, JSON_FALSE));
  json_insert(object, SV("ccc"),
              json_new(pool, JSON_NUMBER, (json_number_t)100));
  return object;
}

json_node_t check_array_construct(json_pool_t *pool) {
  printf(INFO("--- Test check_array_construct"));

  json_node_t array = json_new(pool, JSON_ARRAY);
  for (int i = 1; i <= 5; ++i) {
    json_append(array, json_new(pool, JSON_NUMBER, (json_number_t)i));
  }
  return array;
}

int main(void) {
  run_test(check_new_null, SV("null"));
  run_test(check_new_true, SV("true"));
  run_test(check_new_false, SV("false"));
  run_test(check_new_string, SV("\"Hello, world!\""));
  run_test(check_new_number, SV("42"));
  run_test(check_new_array, SV("[]"));
  run_test(check_new_object, SV("{}"));
  run_test(check_object_construct,
           SV("{\"aaa\":true,\"bbb\":false,\"ccc\":100}"));
  run_test(check_array_construct, SV("[1,2,3,4,5]"));
  run_test(check_parse_true, SV("true"));
  run_test(check_parse_false, SV("false"));
  run_test(check_parse_null, SV("null"));
  run_test(check_parse_number, SV("3.14"));
  run_test(check_parse_string, SV("\"\\\"Hello,\\\\ world!\\\"\""));
  run_test(check_parse_empty_array, SV("[]"));
  run_test(check_parse_singleton_array, SV("[1]"));
  run_test(check_parse_array, SV("[1,2,3,4]"));
  run_test(check_parse_empty_object, SV("{}"));
  run_test(check_parse_singleton_object, SV("{\"Foxy\":true}"));
  run_test(check_parse_object, SV("{\"Cain\":2,\"Able\":1,\"Differ\":3}"));
  return 0;
}
