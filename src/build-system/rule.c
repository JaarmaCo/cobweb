#include <limits.h>

#include "rule.h"

static bool rule_input_from_json(rule_t *rule, json_node_t node, FILE *status, allocator_t *allocator) {
    
    json_string_t contents;
    if (json_get(node, JSON_STRING, &contents)) {
        string_builder_t s = {.allocator = allocator};
        sb_append_sv(&s, sb_view(&contents));
        return true;
    }

    json_array_t array;
    if (!json_get(node, JSON_ARRAY, &array)) {
        fputs("\"input\" has to be a string, or an array of strings.", status);
        return false;
    }

    for (size_t i = 0; i < array.count; ++i) {
        json_node_t child = array.items[i];
        if (!json_get(child, JSON_STRING, &contents)) {
            fprintf(status, "Invalid value at element %zu in \"input\", expected a string.\n", i);
            return false;
        }
        string_builder_t sb = {.allocator = allocator};
        sb_append_sv(&sb, sb_view(&contents));
        file_list_append(&rule->input, sb);
    }
    return true;
}

static bool rule_output_from_json(rule_t *rule, json_node_t node, FILE *status, allocator_t *allocator) {
    
    json_string_t contents;
    if (json_get(node, JSON_STRING, &contents)) {
        string_builder_t s = {.allocator = allocator};
        sb_append_sv(&s, sb_view(&contents));
        return true;
    }

    json_array_t array;
    if (!json_get(node, JSON_ARRAY, &array)) {
        fputs("\"output\" has to be a string, or an array of strings.", status);
        return false;
    }

    for (size_t i = 0; i < array.count; ++i) {
        json_node_t child = array.items[i];
        if (!json_get(child, JSON_STRING, &contents)) {
            fprintf(status, "Invalid value at element %zu in \"output\", expected a string.\n", i);
            return false;
        }
        string_builder_t sb = {.allocator = allocator};
        sb_append_sv(&sb, sb_view(&contents));
        file_list_append(&rule->output, sb);
    }
    return true;
}

static bool rule_flags_from_json(rule_t *rule, json_node_t node, FILE *status, allocator_t *allocator) {
    
    json_array_t array;
    json_string_t contents;
    if (!json_get(node, JSON_ARRAY, &array)) {
        fputs("\"flags\" has to be an array of strings.", status);
        return false;
    }

    for (size_t i = 0; i < array.count; ++i) {
        json_node_t child = array.items[i];
        if (!json_get(child, JSON_STRING, &contents)) {
            fprintf(status, "Invalid value at element %zu in \"flags\", expected a string.\n", i);
            return false;
        }
        string_builder_t sb = {.allocator = allocator};
        sb_append_sv(&sb, sb_view(&contents));
        flags_append(&rule->flags, sb);
    }
    return true;
}

static bool rule_depends_from_json(rule_t *rule, json_node_t node, FILE *status, allocator_t *allocator) {
    
    json_array_t array;
    json_number_t contents;
    if (!json_get(node, JSON_ARRAY, &array)) {
        fputs("\"depends\" has to be an array of integers.", status);
        return false;
    }

    for (size_t i = 0; i < array.count; ++i) {
        json_node_t child = array.items[i];
        if (!json_get(child, JSON_NUMBER, &contents)) {
            fprintf(status, "Invalid value at element %zu in \"depends\", expected a number.\n", i);
            return false;
        }

        if (contents == 0) {
            fprintf(status, "Dependency indices are 1-indexed, "
                    "value at position %zu is 0 (in array \"depends\").\n", i);
            return false;
        }

        if (contents < 0) {
            fprintf(status, "Negative dependency index at position %zu in \"depends\".\n", i);
            return false;
        }

        if (contents > UINT_MAX) {
            fprintf(status, "Dependency index exceeds maximum limit at position %zu in \"depends\".\n", i);
            return false;
        }

        if ((json_number_t)(unsigned int)contents != contents) {
            fprintf(status, "Expected an integer at position %zu in \"depends\".\n", i);
            return false;
        }

        dep_list_append(&rule->depends, (unsigned int)contents);
    }
    return true;
}

static bool rule_linker_from_json(rule_t *rule, json_node_t node, FILE *status, allocator_t *allocator) {
    
    if (node.type != JSON_OBJECT) {
        fputs("Expected property \"linker\" to be an object.", status);
        return false;
    }

    json_string_t string;
    json_array_t array;
    json_node_t *child;

    child = json_find(node, SV("rpath"));
    if (child) {
        if (!json_get(*child, JSON_STRING, &string)) {
            fputs("Expected property \"rpath\" of object \"linker\" to be a string.",
                    status);
            return false;
        }
        string_builder_t sb = {.allocator = allocator};
        sb_append_sv(&sb, sb_view(&string));
        rule->linker.rpath = sb;
    }

    child = json_find(node, SV("directories"));
    if (child) {
        if (!json_get(*child, JSON_ARRAY, &array)) {
            fputs("Expected property \"directories\" of object \"linker\" to be an array of strings.", status);
            return false;
        }

        for (size_t i = 0; i < array.count; ++i) {
            string_builder_t sb = {.allocator = allocator};
            if (!json_get(array.items[i], JSON_STRING, &string)) {
                fprintf(status, "Expected element at position %zu in array "
                        "\"directories\" of object \"linker\" to be a string.\n", i);
                return false;
            }
            sb_append_sv(&sb, sb_view(&string));
            file_list_append(&rule->linker.directories, sb);
        }
    }

    child = json_find(node, SV("libraries"));
    if (child) {
        if (!json_get(*child, JSON_ARRAY, &array)) {
            fputs("Expected property \"libraries\" of object \"linker\" to be an array of strings.", status);
            return false;
        }

        for (size_t i = 0; i < array.count; ++i) {
            string_builder_t sb = {.allocator = allocator};
            if (!json_get(array.items[i], JSON_STRING, &string)) {
                fprintf(status, "Expected element at position %zu in array "
                        "\"libraries\" of object \"linker\" to be a string.\n", i);
                return false;
            }
            sb_append_sv(&sb, sb_view(&string));
            file_list_append(&rule->linker.libraries, sb);
        }
    }

    return true;
}

static bool rule_preprocessor_from_json(rule_t *rule, json_node_t node, FILE *status, allocator_t *allocator) {
    
    if (node.type != JSON_OBJECT) {
        fputs("Expected property \"preprocessor\" to be an object.", status);
        return false;
    }

    json_string_t string;
    json_array_t array;
    json_node_t *child;

    child = json_find(node, SV("includes"));
    if (child) {
        if (!json_get(*child, JSON_ARRAY, &array)) {
            fputs("Expected property \"includes\" of object \"preprocessor\" to be an array of strings.", status);
            return false;
        }

        for (size_t i = 0; i < array.count; ++i) {
            string_builder_t sb = {.allocator = allocator};
            if (!json_get(array.items[i], JSON_STRING, &string)) {
                fprintf(status, "Expected element at position %zu in array "
                        "\"includes\" of object \"preprocessor\" to be a string.\n", i);
                return false;
            }
            sb_append_sv(&sb, sb_view(&string));
            file_list_append(&rule->preprocessor.includes, sb);
        }
    }

    child = json_find(node, SV("definitions"));
    if (child) {
        if (child->type != JSON_OBJECT) {
            fputs("Expected property \"definitions\" of object \"preprocessor\" "
                    "to be an object of symbol-expansion pairs.\n", status);
            return false;
        }

        size_t state = 0;
        string_view_t key;
        json_node_t value;
        while (json_iterate_properties(*child, &state, &key, &value)) {

            preprocessor_definition_t def = {
                .symbol = { .allocator = allocator },
                .expansion = { .allocator = allocator },
            };

            sb_append_sv(&def.symbol, key);

            if (json_get(value, JSON_STRING, &string)) {
                sb_append_sv(&def.expansion, sb_view(&string));
            } else if (value.type != JSON_NULL) {
                fprintf(status, "Expected property \"%.*s\" of object \"definitions\" "
                        "in object \"preprocessor\" to be eithr a string or null.\n", 
                        (int)key.count, key.items);
                return false;
            }

            define_list_append(&rule->preprocessor.definitions, def);
        }
    }
    return true;
}

bool rule_from_json(rule_t *rule, json_node_t node, FILE *status, allocator_t *allocator) {
    
    if (node.type != JSON_OBJECT) {
        fputs("Expected the root element to be an object.", status);
        return false;
    }

    json_node_t *child;
    memset(rule, 0, sizeof *rule);

    rule->allocator = allocator;
    rule->input.allocator = allocator;
    rule->output.allocator = allocator;
    rule->flags.allocator = allocator;
    rule->depends.allocator = allocator;
    rule->linker.rpath.allocator = allocator;
    rule->linker.directories.allocator = allocator;
    rule->linker.libraries.allocator = allocator;
    rule->preprocessor.definitions.allocator = allocator;
    rule->preprocessor.includes.allocator = allocator;

    child = json_find(node, SV("input"));
    if (!child) {
        fputs("Missing required \"input\" property.", status);
        return false;
    }
    if (!rule_input_from_json(rule, *child, status, allocator)) {
        return false;
    }

    child = json_find(node, SV("output"));
    if (child) {
        if (!rule_output_from_json(rule, *child, status, allocator)) {
            return false;
        }
    }

    child = json_find(node, SV("flags"));
    if (child) {
        if (!rule_flags_from_json(rule, *child, status, allocator)) {
            return false;
        }
    }

    child = json_find(node, SV("depends"));
    if (child) {
        if (!rule_depends_from_json(rule, *child, status, allocator)) {
            return false;
        }
    }

    child = json_find(node, SV("linker"));
    if (child) {
        if (!rule_linker_from_json(rule, *child, status, allocator)) {
            return false;
        }
    }

    child = json_find(node, SV("preprocessor"));
    if (child) {
        if (!rule_preprocessor_from_json(rule, *child, status, allocator)) {
            return false;
        }
    }
    return true;
}

bool rule_list_from_json(rule_list_t *list, json_node_t json, FILE *status, allocator_t *allocator) { 
    json_array_t array;
    if (!json_get(json, JSON_ARRAY, &array)) {
        fputs("Expected a list as the root object.\n", status);
        return false;
    }
    memset(list, 0, sizeof *list);
    list->allocator = allocator;
    for (size_t i = 0; i < array.count; ++i) {
        rule_t rule = {0};
        if (!rule_from_json(&rule, array.items[i], status, allocator)) {
            rule_destroy(&rule);
            fprintf(status, "When parsing rule at position %zu.\n", i);
            return false;
        }
        rule_list_append(list, rule);
    }
    return true;
}

void rule_destroy(rule_t *rule) {
    
    if (!rule) {
        return;
    }

    file_list_destroy(&rule->input);
    file_list_destroy(&rule->output);
    flags_destroy(&rule->flags);
    dep_list_destroy(&rule->depends);
    sb_destroy(&rule->linker.rpath);
    file_list_destroy(&rule->linker.directories);
    file_list_destroy(&rule->linker.libraries);
    file_list_destroy(&rule->preprocessor.includes);
    define_list_destroy(&rule->preprocessor.definitions);
}

command_t *run_rule_only(const rule_t *rule, build_context_t *context) {
    return context->compiler.callback(rule, context, context->compiler.arg);
}

command_t *run_rule(unsigned int number, const rule_list_t *rules, build_context_t *context) {
    
    if (number == 0 || number > rules->count) {
        fprintf(context->status, "Rule number %u is outside the valid range [1, %u]\n",
                number, (unsigned int)rules->count);
        return NULL;
    }

    

    return NULL;
}

#define TYPE_0 file_list_t
#define TYPE_1 string_builder_t
#define PREFIX file_list_
#define SUFFIX
#define C_SOURCE
#define FUNCTION_0 sb_destroy
#include "../dynamic_array.h"

#define TYPE_0 dependency_list_t
#define TYPE_1 unsigned int
#define PREFIX dep_list_
#define SUFFIX
#define C_SOURCE
#include "../dynamic_array.h"

#define TYPE_0 preprocessor_definition_list_t
#define TYPE_1 preprocessor_definition_t
#define PREFIX define_list_
#define SUFFIX
#define C_SOURCE
#define FUNCTION_0(def) do { sb_destroy(&(def)->symbol); sb_destroy(&(def)->expansion); } while (0)
#include "../dynamic_array.h"

#define TYPE_0 flags_list_t
#define TYPE_1 string_builder_t
#define PREFIX flags_
#define SUFFIX
#define C_SOURCE
#define FUNCTION_0 sb_destroy
#include "../dynamic_array.h"

#define TYPE_0 rule_list_t
#define TYPE_1 rule_t
#define PREFIX rule_list_
#define SUFFIX
#define C_SOURCE
#define FUNCTION_0 rule_destroy
#include "../dynamic_array.h"


