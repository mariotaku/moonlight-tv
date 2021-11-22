#include "sps_parser.h"

#include <assert.h>

typedef struct bitstream_t {
    const unsigned char *data;
    uint32_t offset;
    uint32_t consecutive_zeroes;
} bitstream_t;

static void bitstream_init(bitstream_t *buf, const unsigned char *data) {
    buf->data = data;
    buf->offset = 0;
    buf->consecutive_zeroes = 0;
}

static uint32_t bitstream_read_bits(bitstream_t *buf, uint32_t size) {
    assert(size <= 32);
    uint32_t result = 0;
    for (uint32_t i = 0; i < size; i++) {
        if ((buf->offset + i) % 8 == 0) {
            unsigned char b = buf->data[(buf->offset + i) / 8];
            if (b == 0) {
                buf->consecutive_zeroes++;
            } else if (b == 0x03 && buf->consecutive_zeroes == 2) {
                buf->offset += 8;
                buf->consecutive_zeroes = buf->data[(buf->offset + i) / 8] == 0;
            } else {
                buf->consecutive_zeroes = 0;
            }
        }
        uint32_t cur_offset = buf->offset + i;
        uint32_t byte_index = cur_offset / 8;
        uint8_t bit_offset = 7 - cur_offset % 8;
        result |= (buf->data[byte_index] >> bit_offset & 0x1) << (size - i - 1);
    }
    buf->offset += size;
    return result;
}

static void bitstream_skip_bits(bitstream_t *buf, uint32_t size) {
    for (int i = 0; i < size; i += 16) {
        bitstream_read_bits(buf, size - i < 16 ? size - i : 16);
    }
}

static uint32_t bitstream_read_ueg(bitstream_t *buf) {
    uint32_t i = 0;
    uint8_t bit;
    uint32_t value;

    bit = bitstream_read_bits(buf, 1);

    while (bit == 0) {
        i++;
        bit = bitstream_read_bits(buf, 1);
    }

    assert (i <= 31);

    value = bitstream_read_bits(buf, i);

    return (1 << i) - 1 + value;
}

static int32_t bitstream_read_eg(bitstream_t *buf) {
    uint32_t value = bitstream_read_ueg(buf);
    if (value & 0x01) {
        return (value + 1) / 2;
    } else {
        return -(value / 2);
    }
}

static void bitstream_skip_scaling_list(bitstream_t *buf, uint8_t count) {
    uint32_t deltaScale, lastScale = 8, nextScale = 8;
    for (uint8_t j = 0; j < count; j++) {
        if (nextScale != 0) {
            deltaScale = bitstream_read_eg(buf);
            nextScale = (lastScale + deltaScale + 256) % 256;
        }
        lastScale = (nextScale == 0 ? lastScale : nextScale);
    }
}

bool sps_parse_dimension_h264(const unsigned char *data, sps_dimension_t *dimension) {
    uint8_t profileIdc = 0;
    uint32_t pict_order_cnt_type = 0;
    uint32_t picWidthInMbsMinus1 = 0;
    uint32_t picHeightInMapUnitsMinus1 = 0;
    uint8_t frameMbsOnlyFlag = 0;
    uint32_t frameCropLeftOffset = 0;
    uint32_t frameCropRightOffset = 0;
    uint32_t frameCropTopOffset = 0;
    uint32_t frameCropBottomOffset = 0;

    bitstream_t buf;
    bitstream_init(&buf, data);
    bitstream_read_bits(&buf, 8);
    profileIdc = bitstream_read_bits(&buf, 8);
    bitstream_read_bits(&buf, 16);
    bitstream_read_ueg(&buf);

    if (profileIdc == 100 || profileIdc == 110 || profileIdc == 122 ||
        profileIdc == 244 || profileIdc == 44 || profileIdc == 83 ||
        profileIdc == 86 || profileIdc == 118 || profileIdc == 128) {
        uint32_t chromaFormatIdc = bitstream_read_ueg(&buf);
        if (chromaFormatIdc == 3) bitstream_read_bits(&buf, 1);
        bitstream_read_ueg(&buf);
        bitstream_read_ueg(&buf);
        bitstream_read_bits(&buf, 1);
        if (bitstream_read_bits(&buf, 1)) {
            for (uint8_t i = 0; i < (chromaFormatIdc != 3) ? 8 : 12; i++) {
                if (bitstream_read_bits(&buf, 1)) {
                    if (i < 6) {
                        bitstream_skip_scaling_list(&buf, 16);
                    } else {
                        bitstream_skip_scaling_list(&buf, 64);
                    }
                }
            }
        }
    }

    bitstream_read_ueg(&buf);
    pict_order_cnt_type = bitstream_read_ueg(&buf);

    if (pict_order_cnt_type == 0) {
        bitstream_read_ueg(&buf);
    } else if (pict_order_cnt_type == 1) {
        bitstream_read_bits(&buf, 1);
        bitstream_read_eg(&buf);
        bitstream_read_eg(&buf);
        for (uint32_t i = 0; i < bitstream_read_ueg(&buf); i++) {
            bitstream_read_eg(&buf);
        }
    }

    bitstream_read_ueg(&buf);
    bitstream_read_bits(&buf, 1);
    picWidthInMbsMinus1 = bitstream_read_ueg(&buf);
    picHeightInMapUnitsMinus1 = bitstream_read_ueg(&buf);
    frameMbsOnlyFlag = bitstream_read_bits(&buf, 1);
    if (!frameMbsOnlyFlag) bitstream_read_bits(&buf, 1);
    bitstream_read_bits(&buf, 1);
    if (bitstream_read_bits(&buf, 1)) {
        frameCropLeftOffset = bitstream_read_ueg(&buf);
        frameCropRightOffset = bitstream_read_ueg(&buf);
        frameCropTopOffset = bitstream_read_ueg(&buf);
        frameCropBottomOffset = bitstream_read_ueg(&buf);
    }
    dimension->width = (((picWidthInMbsMinus1 + 1) * 16) - frameCropLeftOffset * 2 - frameCropRightOffset * 2);
    dimension->height = ((2 - frameMbsOnlyFlag) * (picHeightInMapUnitsMinus1 + 1) * 16) -
                        ((frameMbsOnlyFlag ? 2 : 4) * (frameCropTopOffset + frameCropBottomOffset));
    return true;
}


static int parse_profile_info(bitstream_t *buf) {
    // profile_space:2
    uint32_t profile_space = bitstream_read_bits(buf, 2);
    // tier_flag:1
    uint32_t tier_flag = bitstream_read_bits(buf, 1);
    // profile_idc:5
    uint32_t profile_idc = bitstream_read_bits(buf, 5);

    uint8_t profile_compatibility_flag[32];
    for (int i = 0; i < 32; i++) {
        profile_compatibility_flag[i] = bitstream_read_bits(buf, 1);
    }
    // progressive_source_flag
    bitstream_read_bits(buf, 1);
    // interlaced_source_flag
    bitstream_read_bits(buf, 1);
    // non_packed_constraint_flag
    bitstream_read_bits(buf, 1);
    // frame_only_constraint_flag
    bitstream_read_bits(buf, 1);

    bitstream_skip_bits(buf, 44);

    return 0;
}


bool sps_parse_dimension_hevc(const unsigned char *data, sps_dimension_t *dimension) {
    uint8_t max_sub_layers_minus1 = 0;

    uint8_t sub_layer_profile_present_flag[6];
    uint8_t sub_layer_level_present_flag[6];

    bitstream_t buf;
    bitstream_init(&buf, data);
    // vps_id
    bitstream_read_bits(&buf, 4);
    max_sub_layers_minus1 = bitstream_read_bits(&buf, 3);
    // temporal_id_nesting_flag
    bitstream_read_bits(&buf, 1);
    {
        parse_profile_info(&buf);

        // level_idc
        bitstream_read_bits(&buf, 8);
        for (int i = 0; i < max_sub_layers_minus1; i++) {
            sub_layer_profile_present_flag[i] = bitstream_read_bits(&buf, 1);
            sub_layer_level_present_flag[i] = bitstream_read_bits(&buf, 1);
        }

        if (max_sub_layers_minus1 > 0) {
            for (int i = max_sub_layers_minus1; i < 8; i++) {
                // skip 2 bits
                bitstream_read_bits(&buf, 2);
            }
        }

        for (int i = 0; i < max_sub_layers_minus1; i++) {
            if (sub_layer_profile_present_flag[i]) {
                parse_profile_info(&buf);
            }

            if (sub_layer_level_present_flag[i]) {
                // sub_layer_level_idc[i]
                bitstream_read_bits(&buf, 8);
            }
        }
    }

    // id
    bitstream_read_ueg(&buf);

    uint32_t chroma_format_idc = bitstream_read_ueg(&buf);
    if (chroma_format_idc == 3) {
        // separate_colour_plane_flag:1
        bitstream_read_bits(&buf, 1);
    }

    uint32_t pic_width_in_luma_samples = bitstream_read_ueg(&buf);
    uint32_t pic_height_in_luma_samples = bitstream_read_ueg(&buf);
    if (pic_width_in_luma_samples <= 0 || pic_height_in_luma_samples <= 0) return false;
    dimension->width = pic_width_in_luma_samples;
    dimension->height = pic_height_in_luma_samples;
    return true;
}