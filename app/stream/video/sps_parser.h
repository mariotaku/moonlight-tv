#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct sps_dimension_t {
    uint16_t width;
    uint16_t height;
} sps_dimension_t;

bool sps_parse_dimension_h264(const unsigned char *data, sps_dimension_t *dimension);

bool sps_parse_dimension_hevc(const unsigned char *data, sps_dimension_t *dimension);