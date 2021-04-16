#pragma once
#include "stream/api.h"

#include <pbnjson.h>
#include <resource_calculator_c.h>

#define find_source_port PLUGIN_SYMBOL_NAME(find_source_port)
#define serialize_resource_aquire_req PLUGIN_SYMBOL_NAME(serialize_resource_aquire_req)
#define parse_resource_aquire_resp PLUGIN_SYMBOL_NAME(parse_resource_aquire_resp)

int find_source_port(jvalue_ref res, const char *name);
jvalue_ref serialize_resource_aquire_req(MRCResourceList list);
jvalue_ref parse_resource_aquire_resp(const char *json);