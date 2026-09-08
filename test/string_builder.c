#include "string_builder.h"

#define ESC "\x1B"
#define BOLD ESC "[1m"
#define RED ESC "[31m"
#define GREEN ESC "[32m"
#define BLUE ESC "[34m"
#define RST ESC "[0m"

#define ERR(str) BOLD RED "[ERROR]: " RST str "\n"
#define OK(str) BOLD GREEN "[OK]: " RST str "\n"
#define INFO(str) BOLD BLUE str RST "\n"

static void check_append_char(string_builder_t *sb) {
  printf(INFO("--- Test check_append_char"));
  sb_append_char(sb, 'a');
  sb_append_char(sb, 'b');
  sb_append_char(sb, 'c');
}

static void check_append_sv(string_builder_t *sb) {
  printf(INFO("--- Test check_append_sv"));
  sb_append_sv(sb, SV("The"));
  sb_append_sv(sb, SV(" quick"));
  sb_append_sv(sb, SV(" brown"));
  sb_append_sv(sb, SV(" fox jumped over the lazy dog."));
}

static void check_format(string_builder_t *sb) {
  printf(INFO("-- Test check_format"));
  sb_format(sb, "abc%d", 123);
  sb_format(sb, "def%d", 456);
}

static void check_join(string_builder_t *sb) {
  printf(INFO("--- Test check_join"));
  const char *ITEMS[] = {
      "int", "float", "double", "string", "void",
  };
  sb_append_cstr(sb, "[ ");
  sb_join_cstr(sb, SV(", "), 5, ITEMS);
  sb_append_cstr(sb, " ]");
}

static char RING_OF_FIRE_LYRICS[] =
    "Love is a burning thing\n"
    "And it makes a fiery ring\n"
    "Bound by wild desire\n"
    "I fell into a ring of fire\n"
    "I fell into a burning ring of fire\n"
    "I went down, down, down and flames went higher\n"
    "And it burns, burns, burns\n"
    "The ring of fire, the ring of fire\n";

static void check_read_file(string_builder_t *sb) {
  printf(INFO("--- Test check_read_file"));
  FILE *f = fmemopen(RING_OF_FIRE_LYRICS, sizeof RING_OF_FIRE_LYRICS - 1, "r");
  sb_read_file(sb, f, sizeof "Love is" - 1);
}

static void check_read_line(string_builder_t *sb) {
  printf(INFO("--- Test check_read_line"));
  FILE *f = fmemopen(RING_OF_FIRE_LYRICS, sizeof RING_OF_FIRE_LYRICS - 1, "r");
  sb_read_line(sb, f);
}

static void check_read_until(string_builder_t *sb) {
  printf(INFO("--- Test check_read_until"));
  FILE *f = fmemopen(RING_OF_FIRE_LYRICS, sizeof RING_OF_FIRE_LYRICS - 1, "r");
  sb_read_until(sb, f, SV(" ring"));
}

static void check_append_string_literal(string_builder_t *sb) {
  printf(INFO("--- Test check_append_string_literal"));
  sb_append_string_literal(sb, SV("\"Did I studder?\"\n\xfe"));
}

static void run_test(void (*runner)(string_builder_t *), const char *expected) {
  size_t allocated = 0;
  debug_allocator_t dba =
      trace_free_balance_allocator(malloc_allocator, &allocated);

  string_builder_t sb = {
      .allocator = &dba.base,
  };

  runner(&sb);

  size_t expected_length = strlen(expected);

  if (sb.count != expected_length) {
    fprintf(stderr,
            ERR("Expected the string length to be %zu, however sb.count = %zu"),
            expected_length, sb.count);
    exit(1);
  }

  if (NULL == sb.items && sb.count != 0) {
    fprintf(stderr, ERR("sb.items == NULL when sb.count != 0, sb.count = %zu"),
            sb.count);
    exit(1);
  }

  if (sb.items[sb.count] != '\0') {
    fprintf(stderr, ERR("The string builder does not end with a '\\0' byte."));
    exit(1);
  }

  if (strncmp(expected, sb.items, expected_length) != 0) {
    string_view_t sv = sb_view(&sb);
    fprintf(stderr, ERR("Expected string to be \"%s\", got \"%.*s\""), expected,
            (int)sv.count, sv.items);
    exit(1);
  }

  printf("sb.count = %zu\n", sb.count);
  printf("sb.capacity = %zu\n", sb.capacity);
  printf("sb.items = \"%.*s\"\n", (int)sb.count, sb.items);

  da_destroy_c(&sb);
  if (allocated > 0) {
    fprintf(stderr, ERR("Leaked %zu bytes"), allocated);
    exit(1);
  }

  printf(OK("The test strings match."));
}

int main(void) {
  run_test(check_append_char, "abc");
  run_test(check_append_sv, "The quick brown fox jumped over the lazy dog.");
  run_test(check_format, "abc123def456");
  run_test(check_join, "[ int, float, double, string, void ]");
  run_test(check_read_file, "Love is");
  run_test(check_read_line, "Love is a burning thing");
  run_test(check_read_until, "Love is a burning thing\n"
                             "And it makes a fiery");
  run_test(check_append_string_literal, "\"\\\"Did I studder\\?\\\"\\n\\xfe\"");
  return 0;
}
