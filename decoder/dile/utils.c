#include "utils.h"
#include <stdio.h>

int find_source_port(jvalue_ref res, const char* name)
{
  int result = 0;
  for (int i = 0; i < jarray_size(res); i++)
  {
    jvalue_ref item = jarray_get(res, i);
    int index = 0;
    if (jstring_equal2(jobject_get(item, J_CSTR_TO_BUF("resource")), J_CSTR_TO_BUF(name)) &&
        jnumber_get_i32(jobject_get(item, J_CSTR_TO_BUF("index")), &index) == 0)
    {
      if (result == 0 || index < result)
        result = index;
    }
  }
  return result;
}

jvalue_ref serialize_resource_aquire_req(MRCResourceList list)
{
  jvalue_ref root = jarray_create(0);
  for (int i = 0; list[i]; i++)
  {
    jarray_append(root, jobject_create_var(jkeyval(J_CSTR_TO_JVAL("resource"), j_cstr_to_jval(list[i]->type)),
                                           jkeyval(J_CSTR_TO_JVAL("qty"), jnumber_create_i32(list[i]->quantity)), NULL));
  }
  return root;
}

jvalue_ref parse_resource_aquire_resp(const char *json)
{
  JSchemaInfo schemaInfo;
  jschema_info_init(&schemaInfo, jschema_all(), NULL, NULL);
  jdomparser_ref parser = jdomparser_create(&schemaInfo, 0);
  jdomparser_feed(parser, json, strlen(json));

  jdomparser_end(parser);
  jvalue_ref result = jdomparser_get_result(parser);
  jdomparser_release(&parser);
  return result;
}