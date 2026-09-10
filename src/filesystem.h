#if !defined(fs_H_)
#define fs_H_

#include "allocator.h"
#include "macro.h"
#include "string_view.h"

M_ENUM(directory_option, //
       DIRECTORY_ONE,    //
       DIRECTORY_RECURSIVE)
typedef enum directory_option directory_option_t;

M_ENUM(file_type,
       FILE_TYPE_DOES_NOT_EXIST,    //
       FILE_TYPE_REGULAR,           //
       FILE_TYPE_DIRECTORY,         //
       FILE_TYPE_SYMBOLIC_LINK,     //
       FILE_TYPE_FIFO_SPECIAL,      //
       FILE_TYPE_BLOCK_SPECIAL,     //
       FILE_TYPE_CHARACTER_SPECIAL, //
       FILE_TYPE_SOCKET,            //
       FILE_TYPE_UNKNOWN)
typedef enum file_type file_type_t;

M_ENUM(file_permissions,
       FILE_PERMISSIONS_READABLE_BIT, //
       FILE_PERMISSIONS_WRITABLE_BIT, //
       FILE_PERMISSIONS_EXECUTABLE_BIT)
typedef enum file_permissions file_permissions_t;

typedef string_view_t path_t;

typedef struct file_info file_info_t;
struct file_info {
  file_type_t file_type;
  struct {
    file_permissions_t user : 2;
    file_permissions_t group : 2;
    file_permissions_t other : 2;
  } file_permissions;
};

typedef struct path_list path_list_t;
struct path_list {
  allocator_t *allocator;
  path_t *items;
  size_t count;
  size_t capacity;
};

/**
 * Pools all allocations made during filesystem operations so they can be freed
 * all at once.
 *
 * Note that any paths that where allocated using this context are destroyed
 * when the context is destroyed.
 */
typedef struct fs_context fs_context_t;

/**
 * Callback for handling filesystem errors.
 *
 * @param context Context where the error occured.
 * @param system_error_code Error code raised by the operation.
 * @param error_message A human-readable error message.
 * @param user A user defined argument.
 */
typedef void (*fs_error_callback_t)(fs_context_t *context,
                                    int system_error_code,
                                    string_view_t error_message, void *user);

/**
 * Create a new filesystem context.
 *
 * @param allocator Allocator to use when allocating memory.
 *
 * @return The new context, or NULL if a memory allocation failed.
 */
fs_context_t *fs_context_create(allocator_t *allocator);

/**
 * Free all memory that was allocated during filesystem operations.
 *
 * @param context Context that was allocated using fs_context_create
 */
void fs_context_destroy(fs_context_t *context);

/**
 * Change the filesystem path that is considered "root" for this filesystem
 * context.
 *
 * @param context Context to rescope.
 * @param new_root Path to prepend to other paths created in this context
 *                 hereafter.
 *
 * @return true on success, false if an error occured.
 */
bool fs_change_root(fs_context_t *context, path_t new_root);

/**
 * Sets a callback that is invoked whenever an error is raised.
 *
 * @param context Context to scope the error callback to.
 * @param callback Error callback function to register.
 * @param arg User-defined argument to pass to the callback.
 */
void fs_set_error_callback(fs_context_t *context, fs_error_callback_t callback,
                           void *arg);

/**
 * Get information about a file.
 *
 * @param context A filesystem context to scope the operation to.
 * @param filename Name of the file to stat.
 *
 * @return A file_info structure containing information about the file.
 */
file_info_t fs_stat(fs_context_t *context, path_t filename);

/**
 * Gets the current working directory of the running process.
 *
 * @param context A filesystem context to scope the operation to.
 * @param[out] out_dir Pointer to a variable that receives the path.
 *
 * @return true on success, false if an error occured.
 */
bool fs_get_cwd(fs_context_t *context, path_t *out_dir);

/**
 * Changes the current working directory of the running process.
 *
 * @param context A filesystem context to scope the operation to.
 * @param new_cwd Path to set as the current working directory.
 *
 * @return true on success, false if an error occured.
 */
bool fs_change_cwd(fs_context_t *context, path_t new_cwd);

/**
 * Gets the absolute path from a relative path.
 *
 * @param context A filesystem context to scope the operation to.
 * @param filename Path to expand to the absolute path.
 * @param[out] out_path Pointer to a variable where the path will be stored.
 *
 * @return true on success, false if an error occured.
 */
bool fs_absolute(fs_context_t *context, path_t filename, path_t *out_path);

/**
 * Relativizes a path using a base directory.
 *
 * @param context A filesystem context to scope the operation to.
 * @param base Base filesystem path to match.
 * @param filename Path to relativize.
 * @param[out] out_path Variable that receives the relativized path.
 *
 * @return true on success, false if an error occured.
 */
bool fs_relative(fs_context_t *context, path_t base, path_t filename,
                 path_t *out_path);

/**
 * Concatenates two filesystem paths.
 *
 * @param context A filesystem context to scope the operation to.
 * @param base Left hand part of the path.
 * @param filename Right hand part of the path.
 * @param[out] out_path Pointer to a variable that receives the concatenated
 *                      path.
 *
 * @return true on success, false if an error occured.
 */
bool fs_join_paths(fs_context_t *context, path_t base, path_t filename,
                   path_t *out_path);

/**
 * Lists all entries of a directory.
 *
 * @param context A filesystem context to scope the operation to.
 * @param filename Directory to list.
 * @param[out] out_list Pointer to a variable that receives
 *                      the list of items.
 *
 * @return true on success, false if an error occured.
 */
bool fs_list_directory(fs_context_t *context, path_t filename,
                       path_list_t *out_list);

/**
 * Expands a globbing pattern.
 *
 * @param context A filesystem context to scope the operation to.
 * @param globbing_pattern Pattern to expand.
 * @param[out] out_glob Pointer to a variable that receives the list of items.
 *
 * @return true on success, false if an error occured.
 */
bool fs_glob(fs_context_t *context, path_t globbing_pattern,
             path_list_t *out_glob);

/**
 * Creates a new directory.
 *
 * @param context A filesystem context to scope the operation to.
 * @param filename Name of the directory to create.
 * @param option Whether to create required subdirectories or not.
 *
 * @return true on success, false if an error occured.
 */
bool fs_create_directory(fs_context_t *context, path_t filename,
                         directory_option_t option);

/**
 * Creates a new file.
 *
 * @param context A filesystem context to scope the operation to.
 * @param filename Name of the file to create.
 * @param traits Filesystem properties to assign to the new file.
 */
bool fs_create_file(fs_context_t *context, path_t filename, file_info_t traits);

/**
 * Deletes a directory.
 *
 * @param context A filesystem context to scope the operation to.
 * @param filename Name of the directory to delete.
 * @param option Whether to permit recursive deletion or not.
 *
 * @return true on success, false if an erro occured.
 */
bool fs_remove_directory(fs_context_t *context, path_t filename,
                         directory_option_t option);

/**
 * Deletes a file.
 *
 * @param context A filesystem context to scope the operation to.
 * @param filename Name of the file to delete.
 *
 * @return true on success, false if an error occured.
 */
bool fs_remove_file(fs_context_t *context, path_t filename);

#define TYPE_0 path_list_t
#define TYPE_1 path_t
#define PREFIX path_list_
#define C_HEADER
#include "dynamic_array.h"

#endif
