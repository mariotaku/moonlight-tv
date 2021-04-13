#pragma once

#include <pbnjson.h>
#include <resource_calculator_c.h>

int find_source_port(jvalue_ref res, const char *name);
jvalue_ref serialize_resource_aquire_req(MRCResourceList list);
jvalue_ref parse_resource_aquire_resp(const char *json);