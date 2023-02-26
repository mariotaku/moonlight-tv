#include "stream/platform.h"

#include <string.h>


#include "util/i18n.h"


const audio_config_entry_t audio_configs[] = {
        {AUDIO_CONFIGURATION_STEREO, "stereo", translatable("Stereo")},
        {AUDIO_CONFIGURATION_51_SURROUND, "5.1ch", translatable("5.1 Surround")},
        {AUDIO_CONFIGURATION_71_SURROUND, "7.1ch", translatable("7.1 Surround")},
};
const size_t audio_config_len = sizeof(audio_configs) / sizeof(audio_config_entry_t);

