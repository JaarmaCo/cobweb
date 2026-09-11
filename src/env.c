#if !defined(UNITY_BUILD)
#include "env.h"
#endif

#include <ctype.h>

#define TYPE_0 env_t
#define TYPE_1 env_node_t
#define TYPE_2 string_builder_t
#define PREFIX env_
#define C_SOURCE
#include "trie.h"

extern char **environ;

static int is_varname(int ch) { return isalnum(ch) || ch == '-' || ch == '<'; }

bool env_define(env_t *env, string_view_t key, string_view_t value) {
  string_builder_t sb = {.allocator = env->allocator};
  if (!sb_append_sv(&sb, value)) {
    return false;
  }
  if (!env_insert(env, key, sb)) {
    sb_destroy(&sb);
    return false;
  }
  return true;
}

bool env_inherit_environ(env_t *env) {

  for (size_t i = 0; environ[i]; ++i) {

    string_view_t entry = sv_cstr(environ[i]);
    string_view_t key = sv_cut_ch(&entry, '=');

    if (!env_define(env, key, entry)) {
      return false;
    }
  }
  return true;
}

bool env_get(env_t *env, string_view_t name_pattern, string_view_t *out_value) {
  string_builder_t out_name = {.allocator = env->allocator};
  if (!env_expand(env, name_pattern, ostream_from_sb(&out_name))) {
    sb_destroy(&out_name);
    return false;
  }

  env_node_t *node = env_find(env, sb_view(&out_name));
  sb_destroy(&out_name);
  if (NULL == node) {
    return false;
  }

  *out_value = sb_view(&node->value);
  return true;
}

#define defer_return(...)                                                      \
  result = (__VA_ARGS__);                                                      \
  goto defer

bool env_exec(env_t *env, string_view_t contents, ostream_t out) {
  bool result = true;

  string_builder_t scratch = {
      .allocator = env->allocator,
  };

  string_builder_t old_args = (string_builder_t){0};
  {
    env_node_t *node = env_find(env, SV("<"));
    if (node) {
      old_args = node->value;
    }
  }

  if (!env_expand(env, contents, ostream_from_sb(&scratch))) {
    defer_return(false);
  }

  contents = sb_view(&scratch);

  string_view_t args = sv_drop_while(contents, is_varname);
  string_view_t function = sv_take(contents, args.items - contents.items);
  args = sv_drop_while(args, isspace);

  if (function.count == 0) {
    defer_return(true);
  }

  string_view_t body = {0};
  {
    env_node_t *node = env_find(env, function);
    if (NULL == node) {
      defer_return(true);
    }
    body = sb_view(&node->value);
  }

  if (!env_define(env, SV("<"), args)) {
    defer_return(false);
  }

  if (!env_expand(env, body, out)) {
    defer_return(false);
  }

defer:
  env_insert(env, SV("<"), old_args);
  sb_destroy(&scratch);
  return result;
}

bool env_expand(env_t *env, string_view_t pattern, ostream_t out) {

  while (pattern.count > 0) {

    if (sv_starts_with_substr(pattern, SV("${"))) {
      // 1. Expand the input
      //
      // 2. Find the variable definition, and expand it
      //
      // If the variable was not found, expand to an empty string

      string_view_t contents = sv_group(&pattern, SV("${"), SV("}"));
      if (contents.count == 0) {
        pattern = sv_drop(pattern, 2);
        continue;
      }

      env_get(env, contents, &contents);
      if (ostream_put_sv(contents, out) < 0) {
        return false;
      }
      continue;
    } else if (sv_starts_with_substr(pattern, SV("$("))) {
      // 1. Expand the input
      //
      // 2. Find the variable name as the first alphanumeric character word
      //    in the group $( ).
      //
      // 3. Define $< as the contents after the name, and expand the contents
      //    of the variable.

      string_view_t contents = sv_group(&pattern, SV("$("), SV(")"));
      if (contents.count == 0) {
        pattern = sv_drop(pattern, 2);
        continue;
      }

      if (!env_exec(env, contents, out)) {
        return false;
      }

      continue;
    } else if (sv_starts_with_char(pattern, '$')) {

      string_view_t key = sv_take_while(sv_drop(pattern, 1), is_varname);

      if (key.count == 0 || sv_equals(key, SV("$$"))) {
        if (!ostream_putc('$', out)) {
          return false;
        }
      } else {
        env_node_t *node = env_find(env, key);
        if (NULL != node) {
          if (ostream_put_sv(sb_view(&node->value), out) < 0) {
            return false;
          }
        }
      }

      pattern = sv_drop(pattern, key.count + 1);
      continue;
    }

    if (!ostream_putc(*pattern.items, out)) {
      return false;
    }
    pattern = sv_drop(pattern, 1);
  }
  return true;
}

static bool dump_env_node(string_builder_t *acc, env_node_t *node,
                          ostream_t out) {
  if (NULL == node) {
    return true;
  }
  if (node->value.items) {
    if (ostream_put_sv(sb_view(acc), out) < 0) {
      return false;
    }

    if (ostream_putc('=', out) < 0) {
      return false;
    }

    if (ostream_puts(sb_cstr(&node->value), out) < 0) {
      return false;
    }
  }

  for (size_t i = 0; i < sizeof node->children / sizeof(env_node_t *); ++i) {
    if (!node->children[i]) {
      continue;
    }

    if (!sb_append_char(acc, (char)i)) {
      return false;
    }

    if (!dump_env_node(acc, node->children[i], out)) {
      return false;
    }

    acc->items[--acc->count] = 0;
  }
  return true;
}

bool env_dump(env_t *env, ostream_t out) {
  string_builder_t sb = {
      .allocator = env->allocator,
  };
  bool result = dump_env_node(&sb, env->root, out);
  sb_destroy(&sb);
  return result;
}
