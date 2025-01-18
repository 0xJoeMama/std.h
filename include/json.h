#ifndef JSON_H
#include <errno.h>
#include <stdbool.h>
#define STRING_IMPL
#include "../../std.h/include/hash_map.h"
#include "../../std.h/include/string.h"

typedef enum {
  JSON_NULL,
  JSON_STRING,
  JSON_NUMBER,
  JSON_BOOL,
  JSON_ARRAY,
  JSON_OBJECT
} JsonType_t;

typedef union JsonValue JsonValue_t;
typedef struct TaggedJsonValue TaggedJsonValue_t;

DA_DECLARE(TaggedJsonValue_t)
HM_DECLARE(String_t, TaggedJsonValue_t)

// an object is just a hashmap of elements
typedef HashMap_t(String_t, TaggedJsonValue_t) JsonObject_t;
// an array is just a dynamic array of elements
typedef DynamicArray_t(TaggedJsonValue_t) JsonArray_t;

int json_parse_string(Str_t *s, String_t *result);
int json_parse_number(Str_t *s, double *res);
int json_parse_boolean(Str_t *s, bool *result);
int json_parse_null(Str_t *s);
int json_parse_array(Str_t *s, JsonArray_t *result);
int json_parse_object(Str_t *s, JsonObject_t *result);
void json_deinit(TaggedJsonValue_t value);
int json_parse_value(Str_t *s, TaggedJsonValue_t *value);
int json_parse(const char *s, TaggedJsonValue_t *value);

#ifdef JSON_IMPL
union JsonValue {
  String_t string;
  double number;
  bool boolean;
  JsonArray_t array;
  JsonObject_t object;
};

struct TaggedJsonValue {
  JsonValue_t el;
  JsonType_t type;
};

HM_IMPL(String_t, TaggedJsonValue_t)
DA_IMPL(TaggedJsonValue_t)

void json_deinit(TaggedJsonValue_t value);

void json_object_kv_deinit(KVPair_t(String_t, TaggedJsonValue_t) entry) {
  s_deinit(&entry.k);
  json_deinit(entry.v);
}

void json_deinit(TaggedJsonValue_t value) {
  switch (value.type) {
  case JSON_NULL:
  case JSON_BOOL:
  case JSON_NUMBER:
    break;
  case JSON_STRING:
    s_deinit(&value.el.string);
    break;
  case JSON_ARRAY:
    da_deinit(TaggedJsonValue_t)(&value.el.array, json_deinit);
    break;
  case JSON_OBJECT:
    hm_deinit(String_t, TaggedJsonValue_t)(&value.el.object,
                                           json_object_kv_deinit);
    break;
  }
}

int handle_escape_sequence(String_t *result, Str_t *s) {
  char unicode_buf[3] = {0};
  int escaped_char = ss_advance_once(s);
  if (!escaped_char)
    return 0;

  switch (escaped_char) {
  case '"':
  case '\\':
  case '/':
    return s_push(result, escaped_char);
  case 'b':
    return s_push(result, '\b');
  case 'f':
    return s_push(result, '\f');
  case 'n':
    return s_push(result, '\n');
  case 'r':
    return s_push(result, '\r');
  case 't':
    return s_push(result, '\t');
  case 'u':
    for (int i = 0; i < 2; i++) {
      // read two characters
      for (int j = 0; j < 2; j++) {
        int next_char = ss_advance_once(s);
        if (!next_char)
          return 0;

        // if any of the digits isn't a hex digit, we fail
        if (!isxdigit(next_char))
          return 0;

        unicode_buf[j] = next_char;
      }

      unicode_buf[2] = '\0';

      char *end;
      errno = 0;
      int res = strtoul(unicode_buf, &end, 16);

      if (end == unicode_buf || errno)
        return 0;

      if (!s_push(result, res))
        return 0;
    }
    return 1;
  default:
    return 0;
  }
}

int json_parse_string(Str_t *s, String_t *result) {
  *s = ss_trim_left(*s);

  if (s->len == 0 || *s->s != '"')
    return 0;

  // TODO: maybe handle this using an enum to differentiate between errors
  if (!s_init(result, 16))
    return 0;

  // skip the " character
  // won't fail
  ss_advance_once(s);
  while (s->len > 0) {
    // won't fail becasue len is at least 1
    int next_char = ss_advance_once(s);

    // filter control characters
    if (next_char >= 0 && next_char < ' ')
      break;

    switch (next_char) {
      // end of string
    case '"':
      return 1;
    case '\\':
      // if we can't properly handle the escape sequence, we have a skill issue
      // and fail
      if (!handle_escape_sequence(result, s)) {
        s_deinit(result);
        return 0;
      }
      break;
    default:
      // otherwise just append to the end of the string
      if (!s_push(result, next_char)) {
        s_deinit(result);
        return 0;
      }
    }
  }

  s_deinit(result);
  return 0;
}

// s needs to be null terminated for this function to work properly
int json_parse_number(Str_t *s, double *res) {
  char *end;
  errno = 0;
  // notice we can safely pass s->s into strtod because it is null terminated
  *res = strtod(s->s, &end);
  if (end == s->s || errno)
    return 0;

  size_t advance_bytes = end - s->s;
  ss_advance(s, advance_bytes);

  return 1;
}

int json_parse_boolean(Str_t *s, bool *result) {
  *s = ss_trim_left(*s);
  Str_t true_string = ss_from_cstring("true");
  Str_t false_string = ss_from_cstring("false");

  if (ss_starts_with(*s, true_string)) {
    *result = true;
    ss_advance(s, true_string.len);
    return 1;
  } else if (ss_starts_with(*s, false_string)) {
    *result = false;
    ss_advance(s, false_string.len);
    return 1;
  } else {
    return 0;
  }
}

int json_parse_null(Str_t *s) {
  *s = ss_trim_left(*s);
  Str_t null = ss_from_cstring("null");
  if (!ss_starts_with(*s, null))
    return 0;

  ss_advance(s, null.len);
  return 1;
}

int json_parse_value(Str_t *s, TaggedJsonValue_t *value);

int json_parse_array(Str_t *s, JsonArray_t *result) {
  *s = ss_trim_left(*s);
  if (s->len == 0 || *s->s != '[')
    return 0;

  if (!da_init(TaggedJsonValue_t)(result, 8))
    return 0;

  ss_advance_once(s);
  while (s->len > 0) {
    *s = ss_trim_left(*s);
    // end of array
    if (*s->s == ']') {
      if (!ss_advance_once(s))
        break;

      // only successfull case
      return 1;
    }

    TaggedJsonValue_t new_value;
    if (!json_parse_value(s, &new_value))
      break;

    if (!da_push(TaggedJsonValue_t)(result, new_value))
      break;

    *s = ss_trim_left(*s);

    // we need to support dangling commas and this code does just that. Notice
    // that this is *NOT* standard JSON
    if (*s->s == ',') {
      // trash the comma
      if (!ss_advance_once(s))
        break;
      // otherwise, if the next character is not a closing bracket, we have
      // failed
    } else if (*s->s != ']') {
      break;
    }
  }

  da_deinit(TaggedJsonValue_t)(result, json_deinit);
  return 0;
}

int json_parse_object(Str_t *s, JsonObject_t *result) {
  *s = ss_trim_left(*s);
  if (s->len == 0 || *s->s != '{')
    return 0;

  if (!hm_init(String_t, TaggedJsonValue_t)(result, 8, s_hash, s_eq))
    return 0;

  ss_advance_once(s);
  while (s->len > 0) {
    *s = ss_trim_left(*s);
    // end of array
    if (*s->s == '}') {
      ss_advance_once(s);
      // only successful case
      return 1;
    }

    String_t key;
    if (!json_parse_string(s, &key))
      break;

    *s = ss_trim_left(*s);
    if (ss_advance_once(s) != ':') {
      s_deinit(&key);
      break;
    }

    *s = ss_trim_left(*s);

    TaggedJsonValue_t new_value;
    if (!json_parse_value(s, &new_value)) {
      s_deinit(&key);
      break;
    }

    if (!hm_put(String_t, TaggedJsonValue_t)(result, key, new_value)) {
      s_deinit(&key);
      break;
    }

    *s = ss_trim_left(*s);

    // we need to support dangling commas and this code does just that. Notice
    // that this is *NOT* standard JSON
    if (*s->s == ',') {
      // trash the comma
      if (!ss_advance_once(s))
        break;
      // otherwise, if the next character is not a closing bracket, we have
      // failed
    } else if (*s->s != '}') {
      break;
    }
  }

  hm_deinit(String_t, TaggedJsonValue_t)(result, json_object_kv_deinit);
  return 0;
}

int json_parse_value(Str_t *s, TaggedJsonValue_t *value) {
  memset(value, 0, sizeof(TaggedJsonValue_t));

  *s = ss_trim(*s);

  if (json_parse_string(s, &value->el.string)) {
    value->type = JSON_STRING;
    return 1;
  } else if (json_parse_number(s, &value->el.number)) {
    value->type = JSON_NUMBER;
    return 1;
  } else if (json_parse_boolean(s, &value->el.boolean)) {
    value->type = JSON_BOOL;
    return 1;
  } else if (json_parse_null(s)) {
    value->type = JSON_NULL;
    return 1;
  } else if (json_parse_array(s, &value->el.array)) {
    value->type = JSON_ARRAY;
    return 1;
  } else if (json_parse_object(s, &value->el.object)) {
    value->type = JSON_OBJECT;
    return 1;
  } else {
    return 0;
  }
}

int json_parse(const char *s, TaggedJsonValue_t *value) {
  Str_t ss = ss_trim(ss_from_cstring(s));
  if (!json_parse_value(&ss, value))
    return 0;

  ss = ss_trim(ss);
  if (ss.len != 0) {
    json_deinit(*value);
    return 0;
  }

  return 1;
}
#endif // JSON_IMPL

#define JSON_H
#endif // JSON_H
