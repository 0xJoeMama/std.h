#include <assert.h>
#define JSON_IMPL
#include "../include/json.h"

int main() {
  TaggedJsonValue_t v;
  const char *s =
      "{\"lala\":{\"choices\":[{\"message\":{\"content\":\"Wrong!\"}},{},false,"
      "true,null,42,42.42,\"\",[]]},\"xmm\":\"what?\\\"},\",\"choices\":[{"
      "\"message\":{\"lala\":null,\"content\":\"/////////\\n: "
      "OK!\\t:\\n\\\\\\\\\\\\\\\\\"}},{\"message\":{\"content\":\"Wrong!\"}}]}";

  assert(json_parse(s, &v));
  return 0;
}
