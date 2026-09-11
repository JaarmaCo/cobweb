#if !defined(UNITY_BUILD)
#include "json.h"
#endif

typedef struct array_state {

  json_node_t array;

} array_state_t;

typedef struct object_state {

  json_string_t current_key;
  json_node_t object;

} object_state_t;

typedef struct parser_frame {

  json_type_t type;
  union {

    array_state_t array_state;
    object_state_t object_state;
  };

} parser_frame_t;

typedef struct state {

  void *f;
  int (*get)(void *);

  // Context stack
  size_t stack_size;
  size_t stack_capacity;
  parser_frame_t *stack_frames;

  json_node_t result;

} state_t;

// Dynamic array implementation for the stack
static bool grow_frame(json_parser_t *parser) {

  json_pool_t *pool = parser->user1;
  allocator_t *allocator = pool->allocator;
  state_t *state = parser->user4;

  if (state->stack_size + 1 <= state->stack_capacity) {
    return true;
  }

  size_t new_capacity = (state->stack_capacity + 1) * 2;
  parser_frame_t *new_stack = allocator_resize(
      allocator, state->stack_frames,
      state->stack_capacity * sizeof(parser_frame_t),
      new_capacity * sizeof(parser_frame_t), _Alignof(parser_frame_t));
  if (NULL == new_stack) {
    return false;
  }

  state->stack_frames = new_stack;
  state->stack_capacity = new_capacity;
  return true;
}

// Push a new context into the parser state.
static bool push_frame(json_parser_t *parser, json_type_t new_type) {

  if (!grow_frame(parser)) {
    return false;
  }

  state_t *state = parser->user4;
  json_pool_t *pool = parser->user1;
  parser_frame_t *frame = &state->stack_frames[state->stack_size++];
  memset(frame, 0, sizeof(parser_frame_t));

  frame->type = new_type;
  if (new_type == JSON_ARRAY) {
    frame->array_state.array = json_new(pool, JSON_ARRAY);
  } else {
    frame->object_state.object = json_new(pool, JSON_OBJECT);
  }
  return true;
}

// Remove the topmost context from the parser state and return it's final value
static json_node_t pop_frame(json_parser_t *parser) {

  state_t *state = parser->user4;
  assert(state->stack_size > 0);

  parser_frame_t frame = state->stack_frames[--state->stack_size];
  if (frame.type == JSON_OBJECT) {
    sb_destroy(&frame.object_state.current_key);
    return frame.object_state.object;
  }
  return frame.array_state.array;
}

// Gets the current parser context
static parser_frame_t *current_frame(json_parser_t *parser) {

  state_t *state = parser->user4;
  assert(state->stack_size > 0);

  return &state->stack_frames[state->stack_size - 1];
}

// Checks if the parser is currently inside a context frame
static bool inside_frame(json_parser_t *parser) {
  state_t *state = parser->user4;
  return state->stack_size > 0;
}

// Checks, or gets the topmost object parser context
static object_state_t *object(json_parser_t *parser) {
  return inside_frame(parser) && current_frame(parser)->type == JSON_OBJECT
             ? &current_frame(parser)->object_state
             : NULL;
}

// Checks or gets the topmost array parser context
static array_state_t *array(json_parser_t *parser) {
  return inside_frame(parser) && current_frame(parser)->type == JSON_ARRAY
             ? &current_frame(parser)->array_state
             : NULL;
}

// Constructs a scalar value based on the current state of the parser
static json_node_t scalar_value(json_parser_t *parser, json_type_t type) {
  json_pool_t *pool = parser->user1;
  switch (type) {
  case JSON_TRUE:
  case JSON_FALSE:
  case JSON_NULL:
    return json_new(pool, type);
  case JSON_STRING: {
    json_string_t str = parser->scratch;
    parser->scratch = (json_string_t){.allocator = pool->allocator};
    return json_new(pool, JSON_STRING, str);
  case JSON_NUMBER: {
    json_number_t number = 0;
    sv_read_ld(sb_view(&parser->scratch), &number, 10);
    sb_destroy(&parser->scratch);
    parser->scratch = (json_string_t){.allocator = pool->allocator};
    return json_new(pool, JSON_NUMBER, number);
  }
  }
  default:
    assert(false && "Unreachable");
    return (json_node_t){0};
  }
}

// Handles event that signal that a value is ready
static bool on_value(json_parser_t *parser, json_node_t value) {
  if (object(parser)) {
    object_state_t *state = object(parser);
    json_insert(state->object, sb_view(&state->current_key), value);
    sb_destroy(&state->current_key);
    return true;
  } else if (array(parser)) {
    array_state_t *state = array(parser);
    json_append(state->array, value);
    return true;
  } else {
    state_t *state = parser->user4;
    state->result = value;
    return true;
  }
}

// Stores the object key by moving the scratch buffer
static bool on_object_key(json_parser_t *parser) {
  assert(object(parser));
  json_pool_t *pool = parser->user1;
  object_state_t *state = object(parser);
  state->current_key = parser->scratch;
  parser->scratch = (json_string_t){.allocator = pool->allocator};
  return true;
}

// Handles parsing events
static bool on_event(json_parser_t *parser, json_parser_event_t event,
                     json_type_t type) {
  switch (event) {
  case JSON_EVENT_SCALAR:
    return on_value(parser, scalar_value(parser, type));
  case JSON_EVENT_OBJECT_END:
  case JSON_EVENT_ARRAY_END:
    return on_value(parser, pop_frame(parser));
  case JSON_EVENT_OBJECT_KEY:
    // Attatch the object key
    return on_object_key(parser);
  case JSON_EVENT_OBJECT_BEGIN:
  case JSON_EVENT_ARRAY_BEGIN:
    // Entering a new context
    return push_frame(parser, type);
  }
  return false;
}

static int get_next(json_parser_t *parser) {
  state_t *state = parser->user4;
  return state->get(state->f);
}

bool json_create_value_parser(json_parser_t *out_parser, json_pool_t *pool,
                              int (*get)(void *), void *f) {
  out_parser->on_event = on_event;
  out_parser->get_next = get_next;
  out_parser->scratch = (string_builder_t){.allocator = pool->allocator};
  out_parser->user1 = pool;
  out_parser->user4 =
      allocator_new(pool->allocator, sizeof(state_t), _Alignof(state_t));
  if (NULL == out_parser->user4) {
    return false;
  }
  state_t *state = out_parser->user4;
  memset(state, 0, sizeof *state);
  state->get = get;
  state->f = f;
  return true;
}

json_node_t json_value_parser_result(const json_parser_t *parser) {
  state_t *state = parser->user4;
  return state->result;
}

void json_destroy_value_parser(json_parser_t *parser) {
  state_t *state = parser->user4;
  json_pool_t *pool = parser->user1;
  // Delete any unused object keys
  for (size_t i = 0; i < state->stack_size; ++i) {
    if (state->stack_frames[i].type != JSON_OBJECT) {
      continue;
    }
    sb_destroy(&state->stack_frames[i].object_state.current_key);
  }
  // Free the stack
  allocator_release(pool->allocator, state->stack_frames,
                    state->stack_capacity * sizeof(parser_frame_t),
                    _Alignof(parser_frame_t));
  // Free the state
  allocator_release(pool->allocator, state, sizeof(state_t), _Alignof(state_t));

  // Free the scratch buffer
  sb_destroy(&parser->scratch);

  memset(parser, 0, sizeof *parser);
}

json_node_t json_load(json_pool_t *pool, FILE *f) {
  json_parser_t parser = {0};
  if (!json_create_value_parser(&parser, pool, (int (*)(void *))fgetc, f)) {
    return (json_node_t){0};
  }
  json_node_t result = {0};
  if (json_parse(&parser)) {
    result = json_value_parser_result(&parser);
  }
  json_destroy_value_parser(&parser);
  return result;
}

static int sv_getc(void *arg) {
  string_view_t *sv = arg;
  if (sv->count == 0) {
    return EOF;
  }
  int item = sv->items[0];
  *sv = sv_drop(*sv, 1);
  return item;
}

json_node_t json_loads(json_pool_t *pool, string_view_t *f) {

  json_parser_t parser = {0};
  if (!json_create_value_parser(&parser, pool, sv_getc, f)) {
    return (json_node_t){0};
  }
  json_node_t result = {0};
  if (json_parse(&parser)) {
    result = json_value_parser_result(&parser);
  }
  json_destroy_value_parser(&parser);
  return result;
}
