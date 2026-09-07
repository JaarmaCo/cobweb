#include "hashmap_sv_i.h"

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

static void check_insert(hashmap_sv_i *hm) {
  printf(INFO("--- Test check_insert"));

  hm_insert_sv_i(hm, SV("aaa"), 1);
  hm_insert_sv_i(hm, SV("bbb"), 2);
  hm_insert_sv_i(hm, SV("ccc"), 3);
  hm_insert_sv_i(hm, SV("ddd"), 4);
  hm_insert_sv_i(hm, SV("eee"), 5);
  hm_insert_sv_i(hm, SV("fff"), 6);
}

static void check_replace(hashmap_sv_i *hm) {
  printf(INFO("--- Test check_replace"));

  hm_insert_sv_i(hm, SV("aaa"), 1);
  hm_insert_sv_i(hm, SV("bbb"), 2);
  hm_insert_sv_i(hm, SV("ccc"), 3);
  hm_insert_sv_i(hm, SV("ddd"), 4);
  hm_insert_sv_i(hm, SV("eee"), 5);
  hm_insert_sv_i(hm, SV("fff"), 6);

  hm_replace_sv_i(hm, SV("ddd"), 999);
  hm_replace_sv_i(hm, SV("ccc"), 111);
}

static void check_put(hashmap_sv_i *hm) {
  printf(INFO("--- Test check_put"));

  hm_put_sv_i(hm, SV("aaa"), 1);
  hm_put_sv_i(hm, SV("bbb"), 2);
  hm_put_sv_i(hm, SV("ccc"), 3);
  hm_put_sv_i(hm, SV("ddd"), 4);
  hm_put_sv_i(hm, SV("ddd"), 999);
  hm_put_sv_i(hm, SV("eee"), 5);
  hm_put_sv_i(hm, SV("fff"), 6);

  hm_put_sv_i(hm, SV("ccc"), 111);
}

static void check_delete(hashmap_sv_i *hm) {
  printf(INFO("--- Test check_delete"));

  hm_insert_sv_i(hm, SV("aaa"), 1);
  hm_insert_sv_i(hm, SV("bbb"), 2);
  hm_insert_sv_i(hm, SV("ccc"), 3);
  hm_insert_sv_i(hm, SV("ddd"), 4);
  hm_insert_sv_i(hm, SV("eee"), 5);
  hm_insert_sv_i(hm, SV("fff"), 6);

  hm_delete_sv_i(hm, SV("bbb"));
  hm_delete_sv_i(hm, SV("fff"));
}

static void run_test(void (*test)(hashmap_sv_i *), size_t expected_count,
                     hashmap_entry_sv_i *expected_entries) {

  size_t allocation_size = 0;
  allocator_t mallocator = malloc_allocator();
  debug_allocator_t dba =
      trace_free_balance_allocator(&mallocator, &allocation_size);

  hashmap_sv_i hm = {
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

    hashmap_entry_sv_i *entry = hm_find_sv_i(&hm, key);
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

  hm_destroy_sv_i(&hm);

  if (allocation_size > 0) {
    fprintf(stderr, ERR("Leaked %zu bytes"), allocation_size);
    exit(1);
  }
}

int main(void) {
  hashmap_entry_sv_i inserts[6] = {
      {.key = SV("aaa"), .value = 1}, {.key = SV("bbb"), .value = 2},
      {.key = SV("ccc"), .value = 3}, {.key = SV("ddd"), .value = 4},
      {.key = SV("eee"), .value = 5}, {.key = SV("fff"), .value = 6},
  };
  run_test(check_insert, 6, inserts);

  hashmap_entry_sv_i replacements[6] = {
      {.key = SV("aaa"), .value = 1},   {.key = SV("bbb"), .value = 2},
      {.key = SV("ccc"), .value = 111}, {.key = SV("ddd"), .value = 999},
      {.key = SV("eee"), .value = 5},   {.key = SV("fff"), .value = 6},
  };
  run_test(check_replace, 6, replacements);

  hashmap_entry_sv_i put_items[6] = {
      {.key = SV("aaa"), .value = 1},   {.key = SV("bbb"), .value = 2},
      {.key = SV("ccc"), .value = 111}, {.key = SV("ddd"), .value = 999},
      {.key = SV("eee"), .value = 5},   {.key = SV("fff"), .value = 6},
  };
  run_test(check_put, 6, put_items);

  hashmap_entry_sv_i delete_items[4] = {
      {.key = SV("aaa"), .value = 1},
      {.key = SV("ccc"), .value = 3},
      {.key = SV("ddd"), .value = 4},
      {.key = SV("eee"), .value = 5},
  };
  run_test(check_delete, 4, delete_items);

  return 0;
}
