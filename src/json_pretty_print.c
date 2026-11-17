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

#include "allocator.h"
#include "json.h"
#include "string_builder.h"

typedef void(*json_pretty_printer_t)(json_node_t, int, int, FILE *);
typedef void(*json_string_printer_t)(string_view_t, FILE *);

static void json_pretty_print_array(json_array_t array, int width, int level, FILE *f, json_pretty_printer_t printer) {

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

        printer(array.items[i], width, level + 1, f);

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

static void json_pretty_print_object(json_node_t object, int width, int level, FILE *f,
        json_pretty_printer_t printer, json_string_printer_t string_printer) {
    
    fputc('{', f);

    size_t i = 0;
    size_t state = 0;
    string_view_t key;
    json_node_t value;
    while (json_iterate_properties(object, &state, &key, &value)) {
       
        if (width > 0) {
            fputc('\n', f);
            for (int j = 0; j < level; ++j) {
                for (int k = 0; k < width; ++k) {
                    fputc(' ', f);
                }
            }
        }

        string_printer(key, f);
        fputs(": ", f);
        printer(value, width, level + 1, f);
    
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
}

void json_pretty_print_string_with_ansi_highlighting(string_view_t str, FILE *f) {
        
    allocator_t *old = scratch_allocator_pop();

    string_builder_t sb = {.allocator = scratch_allocator(str.count + 1)};
    sb_append_string_literal(&sb, str);
    fprintf(f, "\x1b[32m%.*s\x1b[0m", (int)sb.count, sb.items);

    scratch_allocator_restore(old);
}

void json_pretty_print_with_ansi_highlighting(json_node_t node, int width, int level, FILE *f) {
    switch (node.type)
    {
    case JSON_NULL:
        fputs("\x1b[34mnull\x1b[0m", f);
        break;
    case JSON_TRUE:
        fputs("\x1b[34mtrue\x1b[0m", f);
        break;
    case JSON_FALSE:
        fputs("\x1b[34mfalse\x1b[0m", f);
        break;
    case JSON_STRING: {
        json_string_t string = json_as_string(node);
        json_pretty_print_string_with_ansi_highlighting(sb_view(&string), f);
    }
        break;
    case JSON_NUMBER:
        fprintf(f, "\x1b[33m%Lg\x1b[0m", json_as_number(node));
        break;
    case JSON_ARRAY:
        json_pretty_print_array(json_as_array(node), width, level, f, json_pretty_print_with_ansi_highlighting);
        break;
    case JSON_OBJECT:
        json_pretty_print_object(node, width, level, f, json_pretty_print_with_ansi_highlighting, json_pretty_print_string_with_ansi_highlighting);
        break;
    }
}

void json_pretty_print_string(string_view_t str, FILE *f) {
    string_builder_t sb = {0};
    sb_append_string_literal(&sb, str);
    fputs(sb.items, f);
    sb_destroy(&sb);
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
        json_pretty_print_array(json_as_array(node), width, level, f, json_pretty_print);
        break;
    case JSON_OBJECT:
        json_pretty_print_object(node, width, level, f, json_pretty_print, json_pretty_print_string);
        break;
    }
}

