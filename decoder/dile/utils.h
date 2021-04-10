#pragma once

#include <pbnjson.h>

int find_source_port(jvalue_ref res);
jvalue_ref serialize_resource_aquire_req(MRCResourceList list);
jvalue_ref parse_resource_aquire_resp(const char *json);