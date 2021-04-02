#include "utils.h"

int find_source_port(jvalue_ref res)
{
  int result = 0;
  for (int i = 0; i < jarray_size(res); i++)
  {
    jvalue_ref item = jarray_get(res, i);
    int index = 0;
    if (jstring_equal2(jobject_get(item, J_CSTR_TO_BUF("resource")), J_CSTR_TO_BUF("VDEC")) &&
        jnumber_get_i32(jobject_get(item, J_CSTR_TO_BUF("index")), &index) == 0)
    {
      if (result == 0 || index < result)
        result = index;
    }
  }
  return result;
}
