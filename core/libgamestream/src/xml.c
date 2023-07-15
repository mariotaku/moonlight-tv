/*
 * This file is part of Moonlight Embedded.
 *
 * Copyright (C) 2015 Iwan Timmer
 *
 * Moonlight is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Moonlight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Moonlight; if not, see <http://www.gnu.org/licenses/>.
 */

#include "xml.h"
#include "errors.h"
#include "set_error.h"

#include <expat.h>
#include <string.h>
#include <assert.h>

#define STATUS_OK 200

struct xml_query {
    char *memory;
    size_t size;
    int start;
    void *data;
};

static void XMLCALL start_element(void *userData, const char *name, const char **atts);

static void XMLCALL end_element(void *userData, const char *name);

static void XMLCALL start_applist_element(void *userData, const char *name, const char **atts);

static void XMLCALL end_applist_element(void *userData, const char *name);

static void XMLCALL start_mode_element(void *userData, const char *name, const char **atts);

static void XMLCALL end_mode_element(void *userData, const char *name);

static void XMLCALL start_status_element(void *userData, const char *name, const char **atts);

static void XMLCALL end_status_element(void *userData, const char *name);

static void XMLCALL write_cdata(void *userData, const XML_Char *s, int len);

int xml_search(char *data, size_t len, const char *node, char **result) {
    return xml_search_ex(data, len, node, false, result);
}

int xml_search_ex(char *data, size_t len, const char *node, bool required, char **result) {
    struct xml_query search = {.memory = required ? NULL : calloc(1, 1), .data = (void *) node};
    XML_Parser parser = XML_ParserCreate("UTF-8");
    XML_SetUserData(parser, &search);
    XML_SetElementHandler(parser, start_element, end_element);
    XML_SetCharacterDataHandler(parser, write_cdata);
    if (!XML_Parse(parser, data, (int) len, 1)) {
        int code = XML_GetErrorCode(parser);
        const char *error = XML_ErrorString(code);
        XML_ParserFree(parser);
        if (search.memory != NULL) {
            free(search.memory);
        }
        return gs_set_error(GS_INVALID, "XML error %d: %s", code, error);
    }

    XML_ParserFree(parser);
    if (search.memory == NULL && required) {
        return gs_set_error(GS_INVALID, "Cant find value for `%s`", node);
    }
    *result = search.memory;
    return GS_OK;
}

int xml_applist(char *data, size_t len, PAPP_LIST *app_list) {
    struct xml_query query = {0};
    XML_Parser parser = XML_ParserCreate("UTF-8");
    XML_SetUserData(parser, &query);
    XML_SetElementHandler(parser, start_applist_element, end_applist_element);
    XML_SetCharacterDataHandler(parser, write_cdata);
    if (!XML_Parse(parser, data, (int) len, 1)) {
        int code = XML_GetErrorCode(parser);
        const char *error = XML_ErrorString(code);
        XML_ParserFree(parser);
        if (query.memory != NULL) {
            free(query.memory);
        }
        return gs_set_error(GS_INVALID, "XML error %d: %s", code, error);
    }

    XML_ParserFree(parser);
    if (query.memory != NULL) {
        free(query.memory);
    }
    *app_list = (PAPP_LIST) query.data;

    return GS_OK;
}

int xml_modelist(char *data, size_t len, PDISPLAY_MODE *mode_list) {
    struct xml_query query = {0};
    XML_Parser parser = XML_ParserCreate("UTF-8");
    XML_SetUserData(parser, &query);
    XML_SetElementHandler(parser, start_mode_element, end_mode_element);
    XML_SetCharacterDataHandler(parser, write_cdata);
    if (!XML_Parse(parser, data, (int) len, 1)) {
        int code = XML_GetErrorCode(parser);
        const char *error = XML_ErrorString(code);
        XML_ParserFree(parser);
        if (query.memory != NULL) {
            free(query.memory);
        }
        return gs_set_error(GS_INVALID, "XML error %d: %s", code, error);
    }

    XML_ParserFree(parser);
    if (query.memory != NULL) {
        free(query.memory);
    }
    *mode_list = (PDISPLAY_MODE) query.data;

    return GS_OK;

}

int xml_status(char *data, size_t len) {
    int status = 0;
    XML_Parser parser = XML_ParserCreate("UTF-8");
    XML_SetUserData(parser, &status);
    XML_SetElementHandler(parser, start_status_element, end_status_element);
    if (!XML_Parse(parser, data, (int) len, 1)) {
        int code = XML_GetErrorCode(parser);
        const char *error = XML_ErrorString(code);
        XML_ParserFree(parser);
        return gs_set_error(GS_INVALID, "XML error %d: %s", code, error);
    }

    XML_ParserFree(parser);
    return status == STATUS_OK ? GS_OK : GS_ERROR;
}

void start_element(void *userData, const char *name, const char **atts) {
    struct xml_query *search = (struct xml_query *) userData;
    if (strcmp(search->data, name) == 0) {
        search->start++;
    }
}

void end_element(void *userData, const char *name) {
    struct xml_query *search = (struct xml_query *) userData;
    if (strcmp(search->data, name) == 0) {
        search->start--;
    }
}

void start_applist_element(void *userData, const char *name, const char **atts) {
    struct xml_query *search = (struct xml_query *) userData;
    if (strcmp("App", name) == 0) {
        PAPP_LIST app = malloc(sizeof(APP_LIST));
        if (app == NULL) {
            return;
        }

        app->id = 0;
        app->name = NULL;
        app->hdr = 0;
        app->next = (PAPP_LIST) search->data;
        search->data = app;
    } else if (strcmp("ID", name) == 0 || strcmp("AppTitle", name) == 0 || strcmp("IsHdrSupported", name) == 0) {
        search->memory = malloc(1);
        search->size = 0;
        search->start = 1;
    }
}

void end_applist_element(void *userData, const char *name) {
    struct xml_query *search = (struct xml_query *) userData;
    if (search->start) {
        PAPP_LIST list = (PAPP_LIST) search->data;
        if (list == NULL) {
            return;
        }

        if (strcmp("ID", name) == 0) {
            list->id = (int) strtol(search->memory, NULL, 10);
            free(search->memory);
            search->memory = NULL;
        } else if (strcmp("AppTitle", name) == 0) {
            list->name = search->memory;
        } else if (strcmp("IsHdrSupported", name) == 0) {
            list->hdr = (int) strtol(search->memory, NULL, 10);
            free(search->memory);
            search->memory = NULL;
        }
        search->start = 0;
    }
}

void start_mode_element(void *userData, const char *name, const char **atts) {
    struct xml_query *search = (struct xml_query *) userData;
    if (strcmp("DisplayMode", name) == 0) {
        PDISPLAY_MODE mode = calloc(1, sizeof(DISPLAY_MODE));
        if (mode != NULL) {
            mode->next = (PDISPLAY_MODE) search->data;
            search->data = mode;
        }
    } else if (search->data != NULL &&
               (strcmp("Height", name) == 0 || strcmp("Width", name) == 0 || strcmp("RefreshRate", name) == 0)) {
        search->memory = malloc(1);
        search->size = 0;
        search->start = 1;
    }
}

void end_mode_element(void *userData, const char *name) {
    struct xml_query *search = (struct xml_query *) userData;
    if (search->data == NULL || !search->start) {
        return;
    }
    PDISPLAY_MODE mode = (PDISPLAY_MODE) search->data;
    if (strcmp("Width", name) == 0) {
        mode->width = strtol(search->memory, NULL, 10);
    } else if (strcmp("Height", name) == 0) {
        mode->height = strtol(search->memory, NULL, 10);
    } else if (strcmp("RefreshRate", name) == 0) {
        mode->refresh = strtol(search->memory, NULL, 10);
    }

    free(search->memory);
    search->memory = NULL;
    search->start = 0;
}

void start_status_element(void *userData, const char *name, const char **atts) {
    if (strcmp("root", name) != 0) {
        return;
    }
    int *status = (int *) userData;
    for (int i = 0; atts[i]; i += 2) {
        if (strcmp("status_code", atts[i]) == 0) {
            *status = atoi(atts[i + 1]);
        } else if (*status != STATUS_OK && strcmp("status_message", atts[i]) == 0) {
            gs_set_error(GS_FAILED, "Bad status %d: %s", *status, atts[i + 1]);
        }
    }
}

void end_status_element(void *userData, const char *name) {}

void write_cdata(void *userData, const XML_Char *s, int len) {
    struct xml_query *search = (struct xml_query *) userData;
    if (search->start <= 0) {
        return;
    }
    void *allocated = realloc(search->memory, search->size + len + 1);
    assert(allocated != NULL);
    search->memory = allocated;

    memcpy(search->memory + search->size, s, len);
    search->size += len;
    search->memory[search->size] = 0;
}
