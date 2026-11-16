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

#include "rule.h"

#include <string.h>

bool rule_type_from_json(rule_type_t *value,
        json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_ENUM_FROM_JSON_IMPLEMENTATION(rule_type_t,
            (RULE_OBJECT, "object"),
            (RULE_EXECUTABLE, "executable"),
            (RULE_DYNAMIC_LIBRARY, "dynamic-library"),
            (RULE_STATIC_LIBRARY, "static-library"));
}

bool file_list_from_json(file_list_t *value,
        json_node_t json, FILE *status, allocator_t *allocator) {
    
    {
        json_string_t string;
        if (json_get(json, JSON_STRING, &string)) {

            memset(value, 0, sizeof *value);

            string_builder_t sb = {.allocator = allocator};
            sb_append_sv(&sb, sb_view(&string));
           
            file_list_append(value, sb);
            return true;
        }
    }

    JSON_LIST_FROM_JSON_IMPLEMENTATION(file_list_t, string_builder_t,
            string_builder, file_list_append);
} 

bool string_builder_from_json(string_builder_t *sb,
        json_node_t json, FILE *status, allocator_t *allocator) { 
    json_string_t string;
    if (!json_get(json, JSON_STRING, &string)) {
        fputs("Expected a string.\n", status);
        return false;
    }
    sb->allocator = allocator;
    sb->count = 0;
    sb->capacity = 0;
    sb->items = NULL;
    sb_append_sv(sb, sb_view(&string));
    return true;
}

bool dependency_index_from_json(unsigned int *value,
        json_node_t json, FILE *status, allocator_t *allocator) {
    
    (void)allocator;

    json_number_t number;
    if (!json_get(json, JSON_NUMBER, &number)) {
        fputs("Expected a number.\n", status);
        return false;
    }

    if (number != (json_number_t)(unsigned int)number) {
        fputs("Expected an integer.\n", status);
        return false;
    }

    if (number <= 0) {
        fputs("Expected a 1-based reference index (integer).\n", status);
        return false;
    }

    *value = (unsigned int)number;
    return true;
}

bool dependency_list_from_json(dependency_list_t *value,
        json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_LIST_FROM_JSON_IMPLEMENTATION(dependency_list_t,
            unsigned int, dependency_index, dep_list_append);
}

bool compiler_options_from_json(compiler_options_t *value,
        json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_OBJECT_FROM_JSON_IMPLEMENTATION(compiler_options_t,
            (compiler_switches, switches, "switches", T),
            (compiler_warning_level, warnings, "warnings", T),
            (compiler_optimization_level, optimization, "optimization", T),
            (preprocessor_arguments, preprocessor, "preprocessor", T),
            (linker_arguments, linker, "linker", T));
}

bool compiler_switches_bits_from_json(compiler_switches_bits_t *value,
        json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_ENUM_FROM_JSON_IMPLEMENTATION(compiler_switches_bits_t,
            (COMPILER_EMIT_DEBUG_SYMBOLS, "emit-debug-symbols"),
            (COMPILER_TREAT_WARNINGS_AS_ERRORS, "warnings-as-errors"),
            (COMPILER_PIC, "pic"),
            (COMPILER_EMIT_ASSEMBLY, "emit-assembly"),
            (COMPILER_EMIT_PREPROCESSED, "emit-preprocessed"),
            (COMPILER_EMIT_RULE_LIST, "emit-rule-list"));
}

bool compiler_switches_from_json(compiler_switches_t *value,
        json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_FLAGS_FROM_JSON_IMPLEMENTATION(compiler_switches_t,
            compiler_switches_bits_t, compiler_switches_bits);
}

bool compiler_warning_level_from_json(compiler_warning_level_t *value,
        json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_ENUM_FROM_JSON_IMPLEMENTATION(compiler_warning_level_t,
            (COMPILER_WARNINGS_OFF, "off"),
            (COMPILER_WARNINGS_BASIC, "basic"),
            (COMPILER_WARNINGS_MODERATE, "moderate"),
            (COMPILER_WARNINGS_STRICT, "strict"),
            (COMPILER_WARNINGS_PEDANTIC, "pedantic"));
}

bool compiler_optimization_level_from_json(compiler_optimization_level_t *value,
        json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_ENUM_FROM_JSON_IMPLEMENTATION(compiler_optimization_level_t,
            (COMPILER_OPTIMIZATION_OFF, "off"),
            (COMPILER_OPTIMIZATION_LOW, "low"),
            (COMPILER_OPTIMIZATION_MODERATE, "moderate"),
            (COMPILER_OPTIMIZATION_HIGH, "high"),
            (COMPILER_OPTIMIZATION_ADVANCED, "advanced"),
            (COMPILER_OPTIMIZATION_AGRESSIVE, "agressive"));
}

bool preprocessor_arguments_from_json(preprocessor_arguments_t *value,
        json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_OBJECT_FROM_JSON_IMPLEMENTATION(preprocessor_arguments_t,
            (file_list, includes, "includes", T),
            (preprocessor_definition_list, definitions, "definitions", T));
}

bool preprocessor_definition_list_from_json(preprocessor_definition_list_t *value,
        json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_LIST_FROM_JSON_IMPLEMENTATION(preprocessor_definition_list_t,
            preprocessor_definition_t, preprocessor_definition, define_list_append);
}

bool preprocessor_definition_from_json(preprocessor_definition_t *value,
        json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_OBJECT_FROM_JSON_IMPLEMENTATION(preprocessor_definition_t,
            (string_builder, symbol, "symbol", F),
            (string_builder, expansion, "expansion", T));
}

bool linker_arguments_from_json(linker_arguments_t *value,
        json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_OBJECT_FROM_JSON_IMPLEMENTATION(linker_arguments_t,
            (string_builder, rpath, "rpath", T),
            (file_list, directories, "directories", T),
            (file_list, libraries, "libraries", T));
}

bool rule_from_json(rule_t *value,
        json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_OBJECT_FROM_JSON_IMPLEMENTATION(rule_t,
            (rule_type, type, "type", F),
            (file_list, input, "input", F),
            (string_builder, output, "output", F),
            (dependency_list, depends, "depends", T),
            (compiler_options, options, "options", T));
}

bool rule_list_from_json(rule_list_t *value,
        json_node_t json, FILE *status, allocator_t *allocator) {
    JSON_LIST_FROM_JSON_IMPLEMENTATION(rule_list_t,
            rule_t, rule, rule_list_append);
}

json_node_t rule_type_to_json(const rule_type_t *value, json_pool_t *pool) {
    JSON_ENUM_TO_JSON_IMPLEMENTATION(rule_type_t,
            (RULE_OBJECT, "object"),
            (RULE_EXECUTABLE, "executable"),
            (RULE_STATIC_LIBRARY, "static-library"),
            (RULE_DYNAMIC_LIBRARY, "dynamic-library"));
}

json_node_t file_list_to_json(const file_list_t *value, json_pool_t *pool) {
    JSON_LIST_TO_JSON_IMPLEMENTATION(string_builder);
}

json_node_t string_builder_to_json(const string_builder_t *value, json_pool_t *pool) {
    string_builder_t sb = {.allocator = pool->allocator};
    sb_append_sv(&sb, sb_view(value));
    return json_new_string(pool, sb);
}

json_node_t dependency_index_to_json(const unsigned int *value, json_pool_t *pool) {
    return json_new_number(pool, (json_number_t)*value);
}

json_node_t dependency_list_to_json(const dependency_list_t *value, json_pool_t *pool) {
    JSON_LIST_TO_JSON_IMPLEMENTATION(dependency_index);
}

json_node_t compiler_options_to_json(const compiler_options_t *value, json_pool_t *pool) {
    JSON_OBJECT_TO_JSON_IMPLEMENTATION(
            (compiler_switches, switches, "switches", T),
            (compiler_warning_level, warnings, "warnings", T),
            (compiler_optimization_level, optimization, "optimization", T),
            (preprocessor_arguments, preprocessor, "preprocessor", T),
            (linker_arguments, linker, "linker", T));
}

json_node_t compiler_switches_bits_to_json(const compiler_switches_bits_t *value, json_pool_t *pool) {

    JSON_ENUM_TO_JSON_IMPLEMENTATION(compiler_switches_bits_t,
            (COMPILER_EMIT_DEBUG_SYMBOLS, "emit-debug-symbols"),
            (COMPILER_TREAT_WARNINGS_AS_ERRORS, "warnings-as-errors"),
            (COMPILER_PIC, "pic"),
            (COMPILER_EMIT_ASSEMBLY, "emit-assembly"),
            (COMPILER_EMIT_PREPROCESSED, "emit-preprocessed"),
            (COMPILER_EMIT_RULE_LIST, "emit-rule-list"));
}

json_node_t compiler_switches_to_json(const compiler_switches_t *value, json_pool_t *pool) {

    JSON_FLAGS_TO_JSON_IMPLEMENTATION(compiler_switches_t, compiler_switches_bits_t,
            (COMPILER_EMIT_DEBUG_SYMBOLS, "emit-debug-symbols"),
            (COMPILER_TREAT_WARNINGS_AS_ERRORS, "warnings-as-errors"),
            (COMPILER_PIC, "pic"),
            (COMPILER_EMIT_ASSEMBLY, "emit-assembly"),
            (COMPILER_EMIT_PREPROCESSED, "emit-preprocessed"),
            (COMPILER_EMIT_RULE_LIST, "emit-rule-list"));
}

json_node_t compiler_warning_level_to_json(const compiler_warning_level_t *value, json_pool_t *pool) {

    JSON_ENUM_TO_JSON_IMPLEMENTATION(compiler_warning_level_t,
            (COMPILER_WARNINGS_OFF, "off"),
            (COMPILER_WARNINGS_BASIC, "basic"),
            (COMPILER_WARNINGS_MODERATE, "moderate"),
            (COMPILER_WARNINGS_STRICT, "strict"),
            (COMPILER_WARNINGS_PEDANTIC, "pedantic"));
}

json_node_t compiler_optimization_level_to_json(const compiler_optimization_level_t *value, json_pool_t *pool) {
    
    JSON_ENUM_TO_JSON_IMPLEMENTATION(compiler_optimization_level_t,
            (COMPILER_OPTIMIZATION_OFF, "off"),
            (COMPILER_OPTIMIZATION_LOW, "low"),
            (COMPILER_OPTIMIZATION_MODERATE, "moderate"),
            (COMPILER_OPTIMIZATION_HIGH, "high"),
            (COMPILER_OPTIMIZATION_ADVANCED, "advanced"),
            (COMPILER_OPTIMIZATION_AGRESSIVE, "agressive"));
}

json_node_t preprocessor_arguments_to_json(const preprocessor_arguments_t *value, json_pool_t *pool) {

    JSON_OBJECT_TO_JSON_IMPLEMENTATION(
            (file_list, includes, "includes", T),
            (preprocessor_definition_list, definitions, "definitions", T));
}

json_node_t preprocessor_definition_list_to_json(const preprocessor_definition_list_t *value, json_pool_t *pool) {
    JSON_LIST_TO_JSON_IMPLEMENTATION(preprocessor_definition);
}

json_node_t preprocessor_definition_to_json(const preprocessor_definition_t *value, json_pool_t *pool) {

    JSON_OBJECT_TO_JSON_IMPLEMENTATION(
            (string_builder, symbol, "symbol", F),
            (string_builder, expansion, "expansion", T));
}

json_node_t linker_arguments_to_json(const linker_arguments_t *value, json_pool_t *pool) {

    JSON_OBJECT_TO_JSON_IMPLEMENTATION(
            (string_builder, rpath, "rpath", T),
            (file_list, directories, "directories", T),
            (file_list, libraries, "libraries", T));
}

json_node_t rule_to_json(const rule_t *value, json_pool_t *pool) {

    JSON_OBJECT_TO_JSON_IMPLEMENTATION(
            (rule_type, type, "type", F),
            (file_list, input, "input", F),
            (string_builder, output, "output", F),
            (dependency_list, depends, "depends", T),
            (compiler_options, options, "options", T));
}

json_node_t rule_list_to_json(const rule_list_t *value, json_pool_t *pool) {
    
    JSON_LIST_TO_JSON_IMPLEMENTATION(rule);
}



