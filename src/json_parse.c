#include "json.h"

#include <ctype.h>

static void next(json_parser_t *parser) {
  int ch = parser->get_next(parser);
  parser->current_char = ch == 0 ? -1 : ch;
  if (ch == '\n') {
    ++parser->lineno.line;
    parser->lineno.column = 1;
  } else {
    ++parser->lineno.column;
  }
  ++parser->lineno.offset;
}

static int current(json_parser_t *parser) {
  if (parser->current_char == 0) {
    next(parser);
  }
  return parser->current_char;
}

static void skip_spaces(json_parser_t *parser) {
  while (isspace(current(parser))) {
    next(parser);
  }
}

static void warn(json_parser_t *parser, int error_code, const char *fmt, ...) {
  if (parser->on_error) {

    char message[512] = {0};

    va_list va;
    va_start(va, fmt);
    vsnprintf(message, sizeof message, fmt, va);
    va_end(va);

    parser->on_error(parser, error_code, sv_cstr(message));
  }
}

static bool error(json_parser_t *parser, int error_code, const char *fmt, ...) {
  if (parser->on_error) {

    char message[512] = {0};

    va_list va;
    va_start(va, fmt);
    vsnprintf(message, sizeof message, fmt, va);
    va_end(va);

    parser->on_error(parser, error_code, sv_cstr(message));
  }
  return false;
}

static bool alloc_error(json_parser_t *parser) {
  return error(parser, JSON_ERROR_MEMORY_ALLOCATION_FAILED,
               "Failed to allocate storage for the temporary buffer.");
}

static bool event(json_parser_t *parser, json_parser_event_t event,
                  json_type_t type) {
  if (parser->on_event) {
    parser->on_event(parser, event, type);
  }
  return true;
}

bool json_parse(json_parser_t *parser) {
  skip_spaces(parser);
  switch (current(parser)) {
  case 't':
  case 'f':
  case 'n':
    return json_parse_keyword(parser);
  case '"':
    return json_parse_string(parser);
  case '[':
    return json_parse_array(parser);
  case '{':
    return json_parse_object(parser);
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
  case '.':
    return json_parse_number(parser);
  }
  return error(parser, JSON_ERROR_UNRECOGNIZED_TOKEN,
               "Unexpected character '\\x%02x' at start of sequence",
               (unsigned int)current(parser));
}

bool json_parse_string_no_event(json_parser_t *parser) {
  skip_spaces(parser);

  if (current(parser) != '"') {
    return error(
        parser, JSON_ERROR_INVALID_START_OF_STRING,
        "Expected '\\x%02x' ('\"') at the start of the string, got '\\x%02x'",
        (unsigned int)'"', (unsigned int)current(parser));
  }
  next(parser);

  parser->scratch.count = 0;
  while (current(parser) != '"') {

    if (current(parser) == EOF) {
      return error(parser, JSON_ERROR_UNEXPECTED_EOF,
                   "String was not terminated before end of input.");
    }

    if (current(parser) == '\\') {
      next(parser);
      switch (current(parser)) {
      case '"':
        if (!sb_append_char(&parser->scratch, '"')) {
          return alloc_error(parser);
        }
        break;
      case '\\':
        if (!sb_append_char(&parser->scratch, '\\')) {
          return alloc_error(parser);
        }
        break;
      case '/':
        if (!sb_append_char(&parser->scratch, '/')) {
          return alloc_error(parser);
        }
        break;
      case 'b':
        if (!sb_append_char(&parser->scratch, '\b')) {
          return alloc_error(parser);
        }
        break;
      case 'f':
        if (!sb_append_char(&parser->scratch, '\f')) {
          return alloc_error(parser);
        }
        break;
      case 'n':
        if (!sb_append_char(&parser->scratch, '\n')) {
          return alloc_error(parser);
        }
        break;
      case 'r':
        if (!sb_append_char(&parser->scratch, '\r')) {
          return alloc_error(parser);
        }
        break;
      case 't':
        if (!sb_append_char(&parser->scratch, '\t')) {
          return alloc_error(parser);
        }
        break;
      case 'u':
        warn(parser, JSON_WARNING_UNICODE_ESCAPE_SEQUENCES_NOT_SUPPORTED,
             "Unicode escape sequences are not supported, replacing with a "
             "space ' '.");
        if (!sb_append_char(&parser->scratch, ' ')) {
          return alloc_error(parser);
        }
        break;
      default:
        return error(parser, JSON_ERROR_INVALID_ESCAPE_SEQUENCE,
                     "'\\x%02x' is not valid in a JSON escape sequence.",
                     (unsigned int)current(parser));
      }
    } else {
      if (!sb_append_char(&parser->scratch, current(parser))) {
        return alloc_error(parser);
      }
    }
    next(parser);
  }
  next(parser);
  return true;
}

bool json_parse_string(json_parser_t *parser) {
  if (json_parse_string_no_event(parser)) {
    return event(parser, JSON_EVENT_SCALAR, JSON_STRING);
  }
  return false;
}

bool json_parse_number(json_parser_t *parser) {
  skip_spaces(parser);

  parser->scratch.count = 0;

  if (current(parser) == '-') {
    next(parser);
    if (!sb_append_char(&parser->scratch, '-')) {
      return alloc_error(parser);
    }
  }

  if (!isdigit(current(parser))) {
    return error(parser, JSON_ERROR_UNRECOGNIZED_TOKEN,
                 "Expected <digit>, got '\\x%02x'.",
                 (unsigned int)current(parser));
  }

  while (current(parser) == '0') {
    next(parser);
  }

  while (isdigit(current(parser))) {

    if (!sb_append_char(&parser->scratch, current(parser))) {
      return alloc_error(parser);
    }
    next(parser);
  }

  if (current(parser) == '.') {

    next(parser);
    if (!sb_append_char(&parser->scratch, '.')) {
      return alloc_error(parser);
    }

    while (isdigit(current(parser))) {
      if (!sb_append_char(&parser->scratch, current(parser))) {
        return alloc_error(parser);
      }
      next(parser);
    }
  }

  if (current(parser) == 'e' || current(parser) == 'E') {

    next(parser);
    if (!sb_append_char(&parser->scratch, 'e')) {
      return alloc_error(parser);
    }

    if (current(parser) == '-' || current(parser) == '+') {
      if (!sb_append_char(&parser->scratch, current(parser))) {
        return alloc_error(parser);
      }
      next(parser);
    }

    while (isdigit(current(parser))) {

      if (!sb_append_char(&parser->scratch, current(parser))) {
        return alloc_error(parser);
      }
      next(parser);
    }
  }
  return event(parser, JSON_EVENT_SCALAR, JSON_NUMBER);
}

static bool read_word(json_parser_t *parser, string_view_t word,
                      json_type_t type) {
  skip_spaces(parser);
  for (size_t i = 0; i < word.count; ++i, next(parser)) {

    if (current(parser) == EOF) {
      return error(
          parser, JSON_ERROR_UNEXPECTED_EOF,
          "Expected keyword %.*s not terminated because of a premature EOF.",
          (int)word.count, word.items);
    }

    if (word.items[i] != current(parser)) {
      return error(parser, JSON_ERROR_UNRECOGNIZED_TOKEN,
                   "Expected keyword %.*s not matched at position %zu. "
                   "Expected '\\x%02x', but got '\\x%02'.",
                   (int)word.count, word.items, i, (unsigned int)word.items[i],
                   (unsigned int)current(parser));
    }
  }
  return event(parser, JSON_EVENT_SCALAR, type);
}

bool json_parse_keyword(json_parser_t *parser) {
  skip_spaces(parser);
  switch (current(parser)) {
  case 't':
    return read_word(parser, SV("true"), JSON_TRUE);
  case 'f':
    return read_word(parser, SV("false"), JSON_FALSE);
  case 'n':
    return read_word(parser, SV("null"), JSON_NULL);
  default:
    return error(parser, JSON_ERROR_UNRECOGNIZED_TOKEN,
                 "Unexpected character '\\x%02x' at start of sequence.",
                 (unsigned int)current(parser));
  }
}

bool json_parse_array(json_parser_t *parser) {
  skip_spaces(parser);
  if (current(parser) != '[') {
    return error(parser, JSON_ERROR_INVALID_START_OF_ARRAY,
                 "Expected '[' at the start of the array.");
  }
  next(parser);
  skip_spaces(parser);

  event(parser, JSON_EVENT_ARRAY_BEGIN, JSON_ARRAY);
  if (current(parser) == ']') {
    next(parser);
    return event(parser, JSON_EVENT_ARRAY_END, JSON_ARRAY);
  }

  if (!json_parse(parser)) {
    return false;
  }

  skip_spaces(parser);
  while (current(parser) != ']') {

    if (current(parser) == EOF) {
      return error(parser, JSON_ERROR_UNEXPECTED_EOF,
                   "Encountered EOF before the array was closed.");
    }

    if (current(parser) != ',') {
      return error(parser, JSON_ERROR_UNRECOGNIZED_TOKEN,
                   "Expected ',' or ']' after the array element.");
    }

    next(parser);
    skip_spaces(parser);

    if (!json_parse(parser)) {
      return false;
    }
    skip_spaces(parser);
  }
  next(parser);
  return event(parser, JSON_EVENT_ARRAY_END, JSON_ARRAY);
}

bool json_parse_object(json_parser_t *parser) {
  skip_spaces(parser);
  if (current(parser) != '{') {
    return error(parser, JSON_ERROR_INVALID_START_OF_OBJECT,
                 "Expected '{' at the start of the object.");
  }
  next(parser);
  skip_spaces(parser);

  event(parser, JSON_EVENT_OBJECT_BEGIN, JSON_OBJECT);
  if (current(parser) == '}') {
    next(parser);
    return event(parser, JSON_EVENT_OBJECT_END, JSON_OBJECT);
  }

  if (!json_parse_string_no_event(parser)) {
    return false;
  }
  event(parser, JSON_EVENT_OBJECT_KEY, JSON_OBJECT);

  skip_spaces(parser);
  if (current(parser) != ':') {
    return error(parser, JSON_ERROR_UNRECOGNIZED_TOKEN,
                 "Expected ':' after object property key.");
  }

  next(parser);

  if (!json_parse(parser)) {
    return false;
  }

  skip_spaces(parser);
  while (current(parser) != '}') {

    if (current(parser) == EOF) {
      return error(parser, JSON_ERROR_UNEXPECTED_EOF,
                   "Encountered EOF before the object was closed.");
    }

    if (current(parser) != ',') {
      return error(parser, JSON_ERROR_UNRECOGNIZED_TOKEN,
                   "Expected ',' or '}' after the object property.");
    }

    next(parser);
    skip_spaces(parser);

    if (!json_parse_string_no_event(parser)) {
      return false;
    }

    event(parser, JSON_EVENT_OBJECT_KEY, JSON_OBJECT);

    skip_spaces(parser);
    if (current(parser) != ':') {
      return error(parser, JSON_ERROR_UNRECOGNIZED_TOKEN,
                   "Expected ':' after object property key.");
    }

    next(parser);
    skip_spaces(parser);

    if (!json_parse(parser)) {
      return false;
    }
    skip_spaces(parser);
  }
  next(parser);
  return event(parser, JSON_EVENT_OBJECT_END, JSON_OBJECT);
}
