#pragma once

#include <stdbool.h>

typedef struct session_input_vmouse_t session_input_vmouse_t;

void session_input_set_vmouse_active(session_input_vmouse_t *vmouse, bool active);

bool session_input_is_vmouse_active(session_input_vmouse_t *vmouse);

void vmouse_set_vector(session_input_vmouse_t *vmouse, short x, short y);

void vmouse_set_trigger(session_input_vmouse_t *vmouse, char l, char r);

void vmouse_set_modifier(session_input_vmouse_t *vmouse, bool v);
