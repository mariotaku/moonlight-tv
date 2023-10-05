/*
 * Copyright (c) 2023 Ningyuan Li <https://github.com/mariotaku>.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "av_pane.h"

#include <stdbool.h>

static bool module_is_auto(const char *value);

void update_conflict_hint(app_t *app, lv_obj_t *hint) {
    SS4S_ModulePreferences preferences = {
            .video_module = app_configuration->decoder,
            .audio_module = app_configuration->audio_backend,
    };
    if (module_is_auto(preferences.video_module) || module_is_auto(preferences.audio_module)) {
        lv_obj_add_flag(hint, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    SS4S_ModuleSelection selection = {
            .audio_module = NULL,
            .video_module = NULL
    };
    SS4S_ModulesSelect(&app->ss4s.modules, &preferences, &selection, false);
    const SS4S_ModuleInfo *vdec = selection.video_module;
    const SS4S_ModuleInfo *adec = selection.audio_module;
    if (vdec != NULL && adec != NULL && SS4S_ModuleInfoConflicts(vdec, adec)) {
        lv_label_set_text_fmt(hint, "%s is conflicting with %s", adec->name, vdec->name);
        lv_obj_clear_flag(hint, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(hint, LV_OBJ_FLAG_HIDDEN);
    }
}

 bool contains_decoder_group(const pref_dropdown_string_entry_t *entry, size_t len, const char *group) {
    for (int i = 0; i < len; i++) {
        if (strcmp(entry[i].value, group) == 0) {
            return true;
        }
    }
    return false;
}

 void set_decoder_entry(pref_dropdown_string_entry_t *entry, const char *name, const char *group, bool fallback) {
    entry->name = name;
    entry->value = group;
    entry->fallback = fallback;
}


static bool module_is_auto(const char *value) {
    return value == NULL || strcmp("auto", value) == 0;
}