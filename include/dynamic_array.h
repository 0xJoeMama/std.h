#ifndef DA_NEW_IMPL
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define FUDGE 2

// needed to force the preprocessor to do cursed stuff
#define da_expand_type(type) DynamicArray_##type##_t
#define DynamicArray_t(type) da_expand_type(type)
#define DynamicArray(type) DynamicArray_##type
#define da_function(ret_type, name, da_type, ...)                              \
  ret_type name##_##da_type(__VA_ARGS__)
#define da_function_call(name, da_type) name##_##da_type

#define DA_DECLARE(type)                                                       \
  struct DynamicArray(type) {                                                  \
    type *buf;                                                                 \
    size_t cap;                                                                \
    size_t len;                                                                \
  };                                                                           \
                                                                               \
  da_function(int, da_init, type, struct DynamicArray(type) * da,              \
              size_t initial_cap);                                             \
  da_function(type *, da_get_raw, type, struct DynamicArray(type) * da,        \
              size_t idx);                                                     \
  da_function(int, da_grow, type, struct DynamicArray(type) * da,              \
              size_t new_cap);                                                 \
  da_function(int, da_push, type, struct DynamicArray(type) * da, type el);    \
  da_function(void, da_deinit, type, struct DynamicArray(type) * da,           \
              void (*destroy)(type));                                          \
  da_function(int, da_shrink, type, struct DynamicArray(type) * da);           \
  da_function(int, da_pop, type, struct DynamicArray(type) * da, type * dst);  \
  da_function(type *, da_get, type, struct DynamicArray(type) * da,            \
              size_t idx);                                                     \
  da_function(void, da_clear, type, struct DynamicArray(type) * da,            \
              void (*destroy)(type));                                          \
  typedef struct DynamicArray(type) DynamicArray_t(type);

#define DA_IMPL(type)                                                          \
  da_function(int, da_init, type, struct DynamicArray(type) * da,              \
              size_t initial_cap) {                                            \
    da->buf = (type *)malloc(initial_cap * sizeof(type));                      \
    if (!da->buf)                                                              \
      return 0;                                                                \
                                                                               \
    da->cap = initial_cap;                                                     \
    da->len = 0;                                                               \
                                                                               \
    return 1;                                                                  \
  }                                                                            \
                                                                               \
  da_function(type *, da_get_raw, type, struct DynamicArray(type) * da,        \
              size_t idx) {                                                    \
    return da->buf + idx;                                                      \
  }                                                                            \
                                                                               \
  da_function(int, da_grow, type, struct DynamicArray(type) * da,              \
              size_t new_cap) {                                                \
    void *new_buf = realloc(da->buf, new_cap * sizeof(type));                  \
    if (!new_buf)                                                              \
      return 0;                                                                \
                                                                               \
    da->buf = (type *)new_buf;                                                 \
    da->cap = new_cap;                                                         \
    return 1;                                                                  \
  }                                                                            \
                                                                               \
  da_function(int, da_push, type, struct DynamicArray(type) * da, type el) {   \
    if (da->len == da->cap) {                                                  \
      /* attempt to grow if we don't have room */                              \
      if (!da_function_call(da_grow, type)(da, FUDGE * da->cap))               \
        return 0;                                                              \
    }                                                                          \
    /* da_get_raw is blind so it might return an invalid address               \
      ignore that case since we know we are in the dynamic array(we resized    \
      *before) */                                                              \
    type *target = da_function_call(da_get_raw, type)(da, da->len);            \
    memcpy(target, &el, sizeof(type));                                         \
    da->len++;                                                                 \
                                                                               \
    return 1;                                                                  \
  }                                                                            \
                                                                               \
  da_function(void, da_deinit, type, struct DynamicArray(type) * da,           \
              void (*destroy)(type)) {                                         \
    if (destroy) {                                                             \
      for (size_t i = 0; i < da->len; i++)                                     \
        destroy(da->buf[i]);                                                   \
    }                                                                          \
                                                                               \
    free(da->buf);                                                             \
    memset(da, 0, sizeof(struct DynamicArray(type)));                          \
  }                                                                            \
                                                                               \
  da_function(int, da_shrink, type, struct DynamicArray(type) * da) {          \
    /* we can't shrink if we have more than half our capacity                  \
    we also can't shrink if we have capacity one because of portability        \
    issues(man realloc) */                                                     \
    if ((da->len >= da->cap / FUDGE) || da->cap <= 1)                          \
      return 1;                                                                \
                                                                               \
    void *new_buf = realloc(da->buf, da->cap / FUDGE * sizeof(type));          \
    if (!new_buf)                                                              \
      return 0;                                                                \
                                                                               \
    da->buf = (type *)new_buf;                                                 \
    da->cap /= FUDGE;                                                          \
                                                                               \
    return 1;                                                                  \
  }                                                                            \
                                                                               \
  da_function(int, da_pop, type, struct DynamicArray(type) * da, type * dst) { \
    /* make sure we have at least 1 element */                                 \
    if (da->len == 0)                                                          \
      return 0;                                                                \
                                                                               \
    da->len = da->len - 1; /* reduce our length */                             \
    /* get the last element */                                                 \
    type *element = da_function_call(da_get_raw, type)(da, da->len);           \
    memcpy(dst, element, sizeof(type)); /* copy it into the dst pointer */     \
                                                                               \
    /* if shrinking fails it means nothing to us since our job is to just      \
    remove an element */                                                       \
    da_function_call(da_shrink, type)(da);                                     \
                                                                               \
    return 1;                                                                  \
  }                                                                            \
                                                                               \
  da_function(void, da_clear, type, struct DynamicArray(type) * da,            \
              void (*destroy)(type)) {                                         \
    if (destroy)                                                               \
      for (size_t i = 0; i < da->len; i++)                                     \
        destroy(da->buf[i]);                                                   \
                                                                               \
    da->len = 0;                                                               \
  }

#define DA_DECLARE_IMPL(type)                                                  \
  DA_DECLARE(type)                                                             \
  DA_IMPL(type)

#define da_init(type) da_function_call(da_init, type)
#define da_get(type) da_function_call(da_get_raw, type)
#define da_push(type) da_function_call(da_push, type)
#define da_pop(type) da_function_call(da_pop, type)
#define da_clear(type) da_function_call(da_clear, type)
#define da_deinit(type) da_function_call(da_deinit, type)

#define DA_NEW_IMPL
#endif
