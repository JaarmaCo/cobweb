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

#if !defined(BUILD_SYSTEM_RULE_H_)

#include "../env.h"
#include "../json.h"
#include "../string_builder.h"
#include "../allocator.h"
#include "../command_builder.h"

#define RULE_MAX_CONCURRENCY 64
#define RULE_MAX_DEPENDENCY_DEPTH 1024

typedef struct rule rule_t;
typedef struct rule_list rule_list_t;
typedef struct file_list file_list_t;
typedef struct dependency_list dependency_list_t;
typedef struct preprocessor_arguments preprocessor_arguments_t;
typedef struct preprocessor_definition preprocessor_definition_t;
typedef struct preprocessor_definition_list preprocessor_definition_list_t;
typedef struct linker_arguments linker_arguments_t;
typedef struct compiler_options compiler_options_t;
typedef struct flags_list flags_list_t;
typedef struct compiler compiler_t;
typedef struct build_context build_context_t;

typedef unsigned int compiler_switches_t;

typedef enum compiler_switches_bits {

    /**
     * Emit debug information in the final objects.
     */
    COMPILER_EMIT_DEBUG_SYMBOLS = 0x1,

    /**
     * Fail compilation if the compiler detects a warning.
     */
    COMPILER_TREAT_WARNINGS_AS_ERRORS = 0x2,

    /**
     * Generate position independent code.
     */
    COMPILER_PIC = 0x4,

    /**
     * Also generate assembly files as a part of the
     * build process.
     */
    COMPILER_EMIT_ASSEMBLY = 0x8,

    /**
     * Also generate preprocessed source files as a part
     * of the build process.
     */
    COMPILER_EMIT_PREPROCESSED = 0x10,

    /**
     * Write a JSON file containing the rule list that was
     * used to build the project.
     */
    COMPILER_EMIT_RULE_LIST = 0x20,

} compiler_switches_bits_t;

typedef enum compiler_warning_level {

    /**
     * Disable warnings
     */
    COMPILER_WARNINGS_OFF = 0,

    /**
     * Use the base level of warnings
     */
    COMPILER_WARNINGS_BASIC = 1,

    /**
     * Apply some additional warnings
     */
    COMPILER_WARNINGS_MODERATE = 2,

    /**
     * Enable stricter warnings and diagnostics
     */
    COMPILER_WARNINGS_STRICT = 3,

    /**
     * All warnings, including pedantic/annoying ones
     */
    COMPILER_WARNINGS_PEDANTIC = 4,

} compiler_warning_level_t;

typedef enum compiler_optimization_level {
    
    /**
     * Disable optimization
     */
    COMPILER_OPTIMIZATION_OFF = 0,

    /**
     * Apply some minor compiler optimizations
     */
    COMPILER_OPTIMIZATION_LOW = 1,

    /**
     * Apply the base level of optimizations
     */
    COMPILER_OPTIMIZATION_MODERATE = 2,

    /**
     * Higher-level of optimization
     */
    COMPILER_OPTIMIZATION_HIGH = 3,

    /**
     * Highest level of optimization that does not change
     * the behavior of the program. Compilation time may
     * be slower.
     */
    COMPILER_OPTIMIZATION_ADVANCED = 4,
    
    /**
     * Ignore build speed entirely and apply all optimizations
     * that are available.
     */
    COMPILER_OPTIMIZATION_AGRESSIVE = 5,

} compiler_optimization_level_t;

typedef enum rule_type {
    RULE_EXECUTABLE,
    RULE_STATIC_LIBRARY,
    RULE_DYNAMIC_LIBRARY,
    RULE_OBJECT,
} rule_type_t;

struct file_list {
    size_t count;
    size_t capacity;
    string_builder_t *items;
    allocator_t *allocator;
};

struct dependency_list {
    size_t count;
    size_t capacity;
    unsigned int *items;
    allocator_t *allocator;
};

struct preprocessor_definition {
    
    /**
     * Preprocessor symbol to add.
     */
    string_builder_t symbol;

    /**
     * String the preprocessor symbol expands to
     * when put to the preprocessor.
     */
    string_builder_t expansion;
};

struct preprocessor_definition_list {
    size_t count;
    size_t capacity;
    preprocessor_definition_t *items;
    allocator_t *allocator;
};

struct preprocessor_arguments {
    
    /**
     * List of directories to add to the preprocessor include paths.
     */
    file_list_t includes;

    /**
     * List of preprocessor definitions to pass to the compilation.
     */
    preprocessor_definition_list_t definitions;
};

struct linker_arguments {

    /**
     * Runtime linking path to use.
     */
    string_builder_t rpath;

    /**
     * List of directories to search when linking to external libraries.
     */
    file_list_t directories;

    /**
     * List of library names to pass to the linker.
     */
    file_list_t libraries;
};

struct compiler_options {
    
    /**
     * General compilation features to enable.
     */
    compiler_switches_t switches;

    /**
     * Abstract warning level of the compiler.
     */
    compiler_warning_level_t warnings;

    /**
     * Abstract optimization level of the compiler.
     */
    compiler_optimization_level_t optimization;

    /**
     * Arguments passed to the preprocessor.
     */
    preprocessor_arguments_t preprocessor;

    /**
     * Arguments passed to the linker.
     */
    linker_arguments_t linker;
};

struct flags_list {
    size_t count;
    size_t capacity;
    string_builder_t *items;
    allocator_t *allocator;
};

struct rule {
    
    /**
     * Specifies the type of artifact this rule produces.
     */
    rule_type_t type;

    /**
     * List of files that has to be present to produce this rule.
     */
    file_list_t input;

    /**
     * Filename of the artifact this rule produces.
     */
    string_builder_t output;

    /**
     * List of 1-based indices that indicates the rules that need
     * to run before this one.
     */
    dependency_list_t depends;
    
    /**
     * Options to pass to the different compilation phases.
     */
    compiler_options_t options;
};

struct rule_list {
    size_t count;
    size_t capacity;
    rule_t *items;
    allocator_t *allocator;
};

/**
 * Callback that translates build-rules into executable commands.
 *
 * Any implementation of this callback is expected to read the properties of
 * the provided rule, and use them to construct a new command in context->command_builder.
 *
 * The manner in which the rule runs is implementation defined, however the
 * final result has to be that the spawned process writes the expected output
 * file. The exact path of this file is determined by reading the rule->output
 * field, which is then prepended by the output directory context->output_directory.
 *
 * @return Either a pointer to the process that was spawned, or NULL to
 *         indicate an error.
 */
typedef command_t *(*compiler_callback_t)(const rule_t *rule, build_context_t *context, void *arg);

struct compiler {
    compiler_callback_t callback;
    void *arg;
};

struct build_context {

    /**
     * Environment used to configure additional options in the build sequence.
     */
    env_t *env;

    /**
     * File stream to write status messages to.
     */
    FILE *status;

    /**
     * Implementation detail, set to 0.
     */
    int dependency_depth;

    /**
     * Level of concurrency to use. Either a non-positive number to run all jobs
     * in sequence, or a number less than or equal to RULE_MAX_CONCURRENCY to run
     * up to that many jobs in parallel.
     */
    int concurreny;

    /**
     * Directory where outputs and intermediate files are placed.
     */
    string_view_t output_directory;

    /**
     * Callbacks used to map rules to actual executable commands.
     */
    compiler_t compiler;

    /**
     * Command builder used to construct build commands. This object
     * must contain valid allocator, and env fields.
     */
    command_builder_t command_builder;

    /**
     * Allocator to use whenever a new one is requested.
     */
    allocator_t *allocator;
};

/**
 * Fill a build_context_t with reasonable default values, readying
 * it for use.
 *
 * This function should only be called on uninitialized or unused 
 * contexts, as the contents of this structure is overwritten by
 * this call.
 *
 * @param env Pointer to an env_t instance that will be used as the
 *        environment of the command builder, and of the context
 *        itself.
 * @param allocator Allocator to use when allocating memory inside
 *        the context.
 */
void build_context_defaults(build_context_t *context, env_t *env, allocator_t *allocator);

/**
 * Run a rule without running transitive dependencies.
 */
command_t *run_rule_only(const rule_t *rule, build_context_t *context);

/**
 * Calls run_rule inside a new process.
 *
 * @return A handle to the forked runner, or NULL to indicate an error.
 */
command_t *run_rule_async(unsigned int number, const rule_list_t *rules, build_context_t *context);

/**
 * Perform the build process by ensuring the provided rule is up to date.
 *
 * @param number 1-based index of the rule to update.
 * @param rules List of rule_t structures containing the descriptions of
 *        how to evaluate each rule.
 * @param context Structure containing contextual information that is passed
 *        to each build step.
 * @return true If the output of the given rule is up to date with it's inputs,
 *         false if the output was missing/out-of-date and could not be produced.
 */
bool run_rule(unsigned int number, const rule_list_t *rules, build_context_t *context);

void preprocessor_arguments_destroy(preprocessor_arguments_t *arguments);

void linker_arguments_destroy(linker_arguments_t *arguments);

void compiler_options_destroy(compiler_options_t *options);

void preprocessor_arguments_inherit(preprocessor_arguments_t *dest, const preprocessor_arguments_t *src,
        allocator_t *allocator);

void linker_arguments_inherit(linker_arguments_t *dest, const linker_arguments_t *src,
        allocator_t *allocator);

void compiler_options_inherit(compiler_options_t *dest, const compiler_options_t *src,
        allocator_t *allocator);

void rule_destroy(rule_t *rule);

static inline compiler_options_t compiler_options(allocator_t *allocator) {
    compiler_options_t result = {
        .switches = 0,
        .warnings = 0,
        .optimization = 0,
        .preprocessor = {
            .definitions = {
                .allocator = allocator,
            },
            .includes = {
                .allocator = allocator,
            },
        },
        .linker = {
            .rpath = {
                .allocator = allocator,
            },
            .libraries = {
                .allocator = allocator,
            },
            .directories = {
                .allocator = allocator,
            }
        }
    };
    return result;
}

bool rule_type_from_json(rule_type_t *value,
        json_node_t json, FILE *status, allocator_t *allocator);

bool file_list_from_json(file_list_t *value,
        json_node_t json, FILE *status, allocator_t *allocator); 

bool string_builder_from_json(string_builder_t *sb,
        json_node_t json, FILE *status, allocator_t *allocator);

bool dependency_index_from_json(unsigned int *index,
        json_node_t json, FILE *status, allocator_t *allocator);

bool dependency_list_from_json(dependency_list_t *list,
        json_node_t json, FILE *status, allocator_t *allocator);

bool compiler_options_from_json(compiler_options_t *options,
        json_node_t json, FILE *status, allocator_t *allocator);

bool compiler_switches_bits_from_json(compiler_switches_bits_t *switches,
        json_node_t json, FILE *status, allocator_t *allocator);

bool compiler_switches_from_json(compiler_switches_t *switches,
        json_node_t json, FILE *status, allocator_t *allocator);

bool compiler_warning_level_from_json(compiler_warning_level_t *level,
        json_node_t json, FILE *status, allocator_t *allocator);

bool compiler_optimization_level_from_json(compiler_optimization_level_t *level,
        json_node_t json, FILE *status, allocator_t *allocator);

bool preprocessor_arguments_from_json(preprocessor_arguments_t *arguments,
        json_node_t json, FILE *status, allocator_t *allocator);

bool preprocessor_definition_list_from_json(preprocessor_definition_list_t *list,
        json_node_t json, FILE *status, allocator_t *allocator);

bool preprocessor_definition_from_json(preprocessor_definition_t *def,
        json_node_t json, FILE *status, allocator_t *allocator);

bool linker_arguments_from_json(linker_arguments_t *arguments,
        json_node_t json, FILE *status, allocator_t *allocator);

bool rule_from_json(rule_t *value,
        json_node_t json, FILE *status, allocator_t *allocator);

bool rule_list_from_json(rule_list_t *list,
        json_node_t json, FILE *status, allocator_t *allocator);

json_node_t rule_type_to_json(const rule_type_t *value, json_pool_t *pool);

json_node_t file_list_to_json(const file_list_t *value, json_pool_t *pool);

json_node_t string_builder_to_json(const string_builder_t *value, json_pool_t *pool);

json_node_t dependency_index_to_json(const unsigned int *value, json_pool_t *pool);

json_node_t dependency_list_to_json(const dependency_list_t *value, json_pool_t *pool);

json_node_t compiler_options_to_json(const compiler_options_t *value, json_pool_t *pool);

json_node_t compiler_switches_bits_to_json(const compiler_switches_bits_t *value, json_pool_t *pool);

json_node_t compiler_switches_to_json(const compiler_switches_t *value, json_pool_t *pool);

json_node_t compiler_warning_level_to_json(const compiler_warning_level_t *value, json_pool_t *pool);

json_node_t compiler_optimization_level_to_json(const compiler_optimization_level_t *value, json_pool_t *pool);

json_node_t preprocessor_arguments_to_json(const preprocessor_arguments_t *value, json_pool_t *pool);

json_node_t preprocessor_definition_list_to_json(const preprocessor_definition_list_t *value, json_pool_t *pool);

json_node_t preprocessor_definition_to_json(const preprocessor_definition_t *value, json_pool_t *pool);

json_node_t linker_arguments_to_json(const linker_arguments_t *value, json_pool_t *pool);

json_node_t rule_to_json(const rule_t *value, json_pool_t *pool);

json_node_t rule_list_to_json(const rule_list_t *value, json_pool_t *pool);

#define TYPE_0 file_list_t
#define TYPE_1 string_builder_t
#define PREFIX file_list_
#define SUFFIX
#define C_HEADER
#include "../dynamic_array.h"

#define TYPE_0 dependency_list_t
#define TYPE_1 unsigned int
#define PREFIX dep_list_
#define SUFFIX
#define C_HEADER
#include "../dynamic_array.h"

#define TYPE_0 preprocessor_definition_list_t
#define TYPE_1 preprocessor_definition_t
#define PREFIX define_list_
#define SUFFIX
#define C_HEADER
#include "../dynamic_array.h"

#define TYPE_0 flags_list_t
#define TYPE_1 string_builder_t
#define PREFIX flags_
#define SUFFIX
#define C_HEADER
#include "../dynamic_array.h"

#define TYPE_0 rule_list_t
#define TYPE_1 rule_t
#define PREFIX rule_list_
#define SUFFIX
#define C_HEADER
#include "../dynamic_array.h"

#define BUILD_SYSTEM_RULE_H_
#endif
