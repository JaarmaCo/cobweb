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

#include "string_view.h"

#define TYPE_2 string_view_t
#define TYPE_3 int
#define PREFIX hm_
#define FUNCTION_1 sv_hash
#define FUNCTION_2 sv_equals
#define HEADER_ONLY
#include "hashmap.h"

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

static void check_insert(hashmap *hm) {
  printf(INFO("--- Test check_insert"));

  hm_insert(hm, SV("aaa"), 1);
  hm_insert(hm, SV("bbb"), 2);
  hm_insert(hm, SV("ccc"), 3);
  hm_insert(hm, SV("ddd"), 4);
  hm_insert(hm, SV("eee"), 5);
  hm_insert(hm, SV("fff"), 6);
}

static void check_replace(hashmap *hm) {
  printf(INFO("--- Test check_replace"));

  hm_insert(hm, SV("aaa"), 1);
  hm_insert(hm, SV("bbb"), 2);
  hm_insert(hm, SV("ccc"), 3);
  hm_insert(hm, SV("ddd"), 4);
  hm_insert(hm, SV("eee"), 5);
  hm_insert(hm, SV("fff"), 6);

  hm_replace(hm, SV("ddd"), 999);
  hm_replace(hm, SV("ccc"), 111);
}

static void check_put(hashmap *hm) {
  printf(INFO("--- Test check_put"));

  hm_put(hm, SV("aaa"), 1);
  hm_put(hm, SV("bbb"), 2);
  hm_put(hm, SV("ccc"), 3);
  hm_put(hm, SV("ddd"), 4);
  hm_put(hm, SV("ddd"), 999);
  hm_put(hm, SV("eee"), 5);
  hm_put(hm, SV("fff"), 6);

  hm_put(hm, SV("ccc"), 111);
}

static void check_delete(hashmap *hm) {
  printf(INFO("--- Test check_delete"));

  hm_insert(hm, SV("aaa"), 1);
  hm_insert(hm, SV("bbb"), 2);
  hm_insert(hm, SV("ccc"), 3);
  hm_insert(hm, SV("ddd"), 4);
  hm_insert(hm, SV("eee"), 5);
  hm_insert(hm, SV("fff"), 6);

  hm_delete(hm, SV("bbb"));
  hm_delete(hm, SV("fff"));
}

static void run_test(void (*test)(hashmap *), size_t expected_count,
                     hashmap_entry *expected_entries) {

  size_t allocation_size = 0;
  debug_allocator_t dba =
      trace_free_balance_allocator(malloc_allocator, &allocation_size);

  hashmap hm = {
      .allocator = &dba.base,
  };

  test(&hm);

  if (NULL == hm.items && hm.count != 0) {
    fprintf(stderr, ERR("hm.items = NULL when hm.count != 0"));
    exit(1);
  }

  if (hm.capacity < hm.count) {
    fprintf(stderr, ERR("Capacity %zu is less than the number of elements %zu"),
            hm.capacity, hm.count);
    exit(1);
  }

  if (hm.count != expected_count) {
    fprintf(stderr, ERR("hm.count (%zu) did not equal the expected count %zu"),
            hm.count, expected_count);
    exit(1);
  }

  for (size_t i = 0; i < hm.capacity; ++i) {
    if (hm.items[i].hash == 0) {
      continue;
    }

    printf("hm[\"%.*s\"] = %d\n", (int)hm.items[i].key.count,
           hm.items[i].key.items, hm.items[i].value);

    size_t j;
    for (j = 0; j < expected_count; ++j) {
      if (sv_equals(expected_entries[j].key, hm.items[i].key)) {
        break;
      }
    }

    if (j == expected_count) {
      fprintf(stderr, ERR("Expected key %.*s is not in the map."),
              (int)expected_entries[j].key.count,
              expected_entries[j].key.items);
      exit(1);
    }

    string_view_t key = expected_entries[j].key;
    int value = expected_entries[j].value;

    memmove(expected_entries + j, expected_entries + j + 1,
            (expected_count - j - 1) * sizeof *expected_entries);
    --expected_count;

    if (value != hm.items[i].value) {
      fprintf(stderr,
              ERR("Found key %.*s, but the values did not match %d != %d"),
              (int)key.count, key.items, hm.items[i].value, value);
      exit(1);
    }

    hashmap_entry *entry = hm_find(&hm, key);
    if (NULL == entry) {
      fprintf(stderr,
              ERR("Could linearly search for key \"%.*s\", but it was not "
                  "findable using hm_find"),
              (int)key.count, key.items);
      exit(1);
    }

    if (entry != &hm.items[i]) {
      fprintf(stderr, ERR("Value of hm_find was not reference-equal to the "
                          "linearly located element"));
      exit(1);
    }
  }

  printf(OK("All entries are in the map"));

  hm_destroy(&hm);

  if (allocation_size > 0) {
    fprintf(stderr, ERR("Leaked %zu bytes"), allocation_size);
    exit(1);
  }
}

int main(void) {
  hashmap_entry inserts[6] = {
      {.key = SV("aaa"), .value = 1}, {.key = SV("bbb"), .value = 2},
      {.key = SV("ccc"), .value = 3}, {.key = SV("ddd"), .value = 4},
      {.key = SV("eee"), .value = 5}, {.key = SV("fff"), .value = 6},
  };
  run_test(check_insert, 6, inserts);

  hashmap_entry replacements[6] = {
      {.key = SV("aaa"), .value = 1},   {.key = SV("bbb"), .value = 2},
      {.key = SV("ccc"), .value = 111}, {.key = SV("ddd"), .value = 999},
      {.key = SV("eee"), .value = 5},   {.key = SV("fff"), .value = 6},
  };
  run_test(check_replace, 6, replacements);

  hashmap_entry put_items[6] = {
      {.key = SV("aaa"), .value = 1},   {.key = SV("bbb"), .value = 2},
      {.key = SV("ccc"), .value = 111}, {.key = SV("ddd"), .value = 999},
      {.key = SV("eee"), .value = 5},   {.key = SV("fff"), .value = 6},
  };
  run_test(check_put, 6, put_items);

  hashmap_entry delete_items[4] = {
      {.key = SV("aaa"), .value = 1},
      {.key = SV("ccc"), .value = 3},
      {.key = SV("ddd"), .value = 4},
      {.key = SV("eee"), .value = 5},
  };
  run_test(check_delete, 4, delete_items);

  return 0;
}
