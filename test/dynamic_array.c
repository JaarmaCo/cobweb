#include "allocator.h"

#define TYPE_1 int
#define PREFIX da_
#define HEADER_ONLY
#include "dynamic_array.h"

#define ESC "\x1B"
#define BOLD ESC "[1m"
#define RED ESC "[31m"
#define GREEN ESC "[32m"
#define BLUE ESC "[34m"
#define RST ESC "[0m"

#define ERR(str) BOLD RED "[ERROR]: " RST str "\n"
#define OK(str) BOLD GREEN "[OK]: " RST str "\n"
#define INFO(str) BOLD BLUE str RST "\n"

static allocator_t ma = {0};

static void run_test(void (*runner)(dynamic_array *da),
                     const int *expected_data, size_t expected_size) {
  size_t allocation_size = 0;
  debug_allocator_t a = trace_free_balance_allocator(&ma, &allocation_size);

  dynamic_array da = {
      .allocator = &a.base,
  };
  runner(&da);

  if (NULL == da.items && da.capacity != 0) {
    fprintf(stderr,
            ERR("Capacity %zu was expected to be zero, as da.items = NULL"),
            da.capacity);
    exit(1);
  }

  if (da.count > da.capacity) {
    fprintf(stderr,
            ERR("Invariant (da.count <= da.capacity) broken: %zu > %zu"),
            da.count, da.capacity);
    exit(1);
  }

  if (da.count != expected_size) {
    fprintf(stderr, ERR("Array size %zu does not equal the expected size %zu"),
            da.count, expected_size);
    exit(1);
  }

  printf("da.count = %zu\n", da.count);
  printf("da.capacity = %zu\n", da.capacity);
  for (size_t i = 0; i < da.count; ++i) {
    printf("da[%zu] = %d\n", i, da.items[i]);
  }

  if (memcmp(da.items, expected_data, expected_size * sizeof(int)) == 0) {
    printf(OK("Matches the expected data."));
  } else {
    fprintf(stderr, ERR("Array contents does not match the expected contents"));
    exit(1);
  }

  da_destroy(&da);

  if (allocation_size > 0) {
    fprintf(stderr, ERR("%zu bytes not freed."), allocation_size);
    exit(1);
  }
}

static void check_append(dynamic_array *da) {
  const int VALUES[] = {
      1, 2, 3, 4, 5, 6, 7,
  };

  printf(INFO("--- Test check_append"));

  for (size_t i = 0; i < sizeof VALUES / sizeof *VALUES; ++i) {
    int *value = da_append(da, VALUES[i]);
    if (NULL == value) {
      fprintf(stderr, ERR("NULL returned from da_append"));
      exit(1);
    }
    if (*value != VALUES[i]) {
      fprintf(stderr,
              ERR("Value at position %zu should equal %d, but equals %d"), i,
              VALUES[i], *value);
      exit(1);
    }
    if (da->count != i + 1) {
      fprintf(stderr, ERR("Inserted %zu elements, but size is %zu"), i + 1,
              da->count);
      exit(1);
    }
  }
}

static void check_append_range(dynamic_array *da) {

  const int VALUES_1[] = {0};
  const int VALUES_2[] = {1, 2, 3};
  const int VALUES_3[] = {4, 5, 6, 7};

  const int *const VALUES[] = {
      VALUES_1,
      VALUES_2,
      VALUES_3,
  };

  const size_t SIZES[] = {
      sizeof VALUES_1 / sizeof(int),
      sizeof VALUES_2 / sizeof(int),
      sizeof VALUES_3 / sizeof(int),
  };

  printf(INFO("--- Test check_append_range"));
  for (size_t i = 0; i < sizeof VALUES / sizeof *VALUES; ++i) {
    size_t old_size = da->count;
    int *first = da_append_range(da, SIZES[i], VALUES[i]);
    if (NULL == first) {
      fprintf(stderr, ERR("NULL returned by da_append_range"));
      exit(1);
    }
    if (memcmp(first, VALUES[i], SIZES[i] * sizeof(int)) != 0) {
      fprintf(stderr, ERR("Append %zu did not match the expected values"), i);
      exit(1);
    }
    if (da->count != old_size + SIZES[i]) {
      fprintf(
          stderr,
          ERR("Appended %zu elements when %zu was expected (iteration %zu)"),
          da->count - old_size, SIZES[i], i);
      exit(1);
    }
  }
}

static void checknsert(dynamic_array *da) {
  printf(INFO("--- Test checknsert"));

  const int VALUES[] = {0, 1, 2, 4, 6, 7};
  da_append_range(da, 6, VALUES);

  int *item = da_insert(da, 3, 3);
  if (NULL == item) {
    fprintf(stderr, ERR("dansert returned NULL"));
    exit(1);
  }
  if (*item != 3) {
    fprintf(stderr, ERR("dansert return value did not correspond to the "
                        "inserted value."));
    exit(1);
  }
  item = da_insert(da, 5, 5);
  if (NULL == item) {
    fprintf(stderr, ERR("dansert returned NULL"));
    exit(1);
  }
  if (*item != 5) {
    fprintf(stderr, ERR("dansert return value did not correspond to the "
                        "inserted value."));
    exit(1);
  }
}

static void check_remove(dynamic_array *da) {
  printf(INFO("--- Test check_remove"));
  const int VALUES[] = {0,   1,   2,   3, 100, 200, 300, 400,
                        500, 600, 700, 4, 5,   6,   7};
  da_append_range(da, sizeof VALUES / sizeof *VALUES, VALUES);
  da_remove_range(da, 4, 7);
}

int main(void) {
  ma = malloc_allocator();
  run_test(check_append, (const int[]){1, 2, 3, 4, 5, 6, 7}, 7);
  run_test(check_append_range, (const int[]){0, 1, 2, 3, 4, 5, 6, 7, 8}, 8);
  run_test(checknsert, (const int[]){0, 1, 2, 3, 4, 5, 6, 7}, 8);
  run_test(check_remove, (const int[]){0, 1, 2, 3, 4, 5, 6, 7}, 8);
  return 0;
}
