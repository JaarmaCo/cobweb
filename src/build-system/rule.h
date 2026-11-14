#if !defined(BUILD_SYSTEM_RULE_H_)

#include "../env.h"
#include "../json.h"
#include "../string_builder.h"
#include "../allocator.h"
#include "../command_builder.h"

#define RULE_MAX_DEPENDENCY_DEPTH 1024

typedef struct rule rule_t;
typedef struct rule_list rule_list_t;
typedef struct file_list file_list_t;
typedef struct dependency_list dependency_list_t;
typedef struct preprocessor_arguments preprocessor_arguments_t;
typedef struct preprocessor_definition preprocessor_definition_t;
typedef struct preprocessor_definition_list preprocessor_definition_list_t;
typedef struct linker_arguments linker_arguments_t;
typedef struct flags_list flags_list_t;
typedef struct compiler compiler_t;
typedef struct build_context build_context_t;

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
    string_builder_t symbol;
    string_builder_t expansion;
};

struct preprocessor_definition_list {
    size_t count;
    size_t capacity;
    preprocessor_definition_t *items;
    allocator_t *allocator;
};

struct preprocessor_arguments {
    file_list_t includes;
    preprocessor_definition_list_t definitions;
};

struct linker_arguments {
    string_builder_t rpath;
    file_list_t directories;
    file_list_t libraries;
};

struct flags_list {
    size_t count;
    size_t capacity;
    string_builder_t *items;
    allocator_t *allocator;
};

struct rule {
    file_list_t input;
    string_builder_t output;
    flags_list_t flags;
    dependency_list_t depends;
    linker_arguments_t linker;
    preprocessor_arguments_t preprocessor;
    allocator_t *allocator;
};

struct rule_list {
    size_t count;
    size_t capacity;
    rule_t *items;
    allocator_t *allocator;
};

typedef command_t *(*compiler_callback_t)(const rule_t *rule, build_context_t *context, void *arg);

struct compiler {
    compiler_callback_t callback;
    void *arg;
};

struct build_context {
    env_t *env;
    FILE *status;
    int dependency_depth;
    int concurreny;
    string_view_t output_directory;
    compiler_t compiler;
    command_builder_t command_builder;
};

void rule_destroy(rule_t *rule);

bool rule_from_json(rule_t *rule, json_node_t json, FILE *status, allocator_t *allocator);

bool rule_list_from_json(rule_list_t *list, json_node_t json, FILE *status, allocator_t *allocator);

command_t *run_rule_only(const rule_t *rule, build_context_t *context);

command_t *run_rule_async(unsigned int number, const rule_list_t *rules, build_context_t *context);

bool run_rule(unsigned int number, const rule_list_t *rules, build_context_t *context);

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
