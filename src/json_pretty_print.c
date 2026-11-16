#include "allocator.h"
#include "json.h"

static void json_pretty_print_array(json_array_t array, int width, int level, FILE *f) {

    fputc('[', f);

    for (size_t i = 0; i < array.count; ++i) {

        if (width > 0) {
            fputc('\n', f);
            for (int j = 0; j < level; ++j) {
                for (int k = 0; k < width; ++k) {
                    fputc(' ', f);
                }
            }
        }

        json_pretty_print(array.items[i], width, level + 1, f);

        if (i != array.count - 1) {
            fputc(',', f);
        } else {
            fputc('\n', f);
            for (int j = 1; j < level; ++j) {
                for (int k = 0; k < width; ++k) {
                    fputc(' ', f);
                }
            }
        }
    }

    fputc(']', f);
}

static void json_pretty_print_object(json_node_t object, int width, int level, FILE *f) {

    allocator_t *old = scratch_allocator_pop();
    
    fputc('{', f);

    size_t i = 0;
    size_t state = 0;
    string_view_t key;
    json_node_t value;
    string_builder_t sb = {.allocator = scratch_allocator(1024)};
    while (json_iterate_properties(object, &state, &key, &value)) {
       
        if (width > 0) {
            fputc('\n', f);
            for (int j = 0; j < level; ++j) {
                for (int k = 0; k < width; ++k) {
                    fputc(' ', f);
                }
            }
        }

        sb.count = 0;
        sb_append_string_literal(&sb, key);

        fprintf(f, "%.*s: ", (int)sb.count, sb.items);
        json_pretty_print(value, width, level + 1, f);
    
        if (i != json_count(object) - 1) {
            fputc(',', f);
        } else {
            fputc('\n', f);
            for (int j = 1; j < level; ++j) {
                for (int k = 0; k < width; ++k) {
                    fputc(' ', f);
                }
            }
        }
        ++i;
    }

    fputc('}', f);
    scratch_allocator_restore(old);
}

void json_pretty_print(json_node_t node, int width, int level, FILE *f) {

    switch (node.type)
    {
    case JSON_NULL:
    case JSON_TRUE:
    case JSON_FALSE:
    case JSON_STRING:
    case JSON_NUMBER:
        json_dumpf(node, f);
        break;
    case JSON_ARRAY:
        json_pretty_print_array(json_as_array(node), width, level, f);
        break;
    case JSON_OBJECT:
        json_pretty_print_object(node, width, level, f);
        break;
    }
}

