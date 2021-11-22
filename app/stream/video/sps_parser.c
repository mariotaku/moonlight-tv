#include "sps_parser.h"

typedef struct bitstream_t {
    const unsigned char *data;
    uint32_t offset;
    uint32_t consecutive_zeroes;
} bitstream_t;

#define EXTENDED_SAR 255

static bool skip_vui_parameters(bitstream_t *buf);

static bool skip_hrd_parameters(bitstream_t *buf);

static int parse_profile_info(bitstream_t *buf);

static void bitstream_init(bitstream_t *buf, const unsigned char *data) {
    buf->data = data;
    buf->offset = 0;
    buf->consecutive_zeroes = 0;
}

static bool bitstream_read_bits(bitstream_t *buf, uint32_t size, uint32_t *value) {
    if (size > 32) return false;
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
    *value = result;
    return true;
}

static bool bitstream_skip_bits(bitstream_t *buf, uint32_t size) {
    uint32_t tmp;
    for (int i = 0; i < size; i += 16) {
        if (!bitstream_read_bits(buf, size - i < 16 ? size - i : 16, &tmp)) return false;
    }
    return true;
}

static inline bool bitstream_read8(bitstream_t *buf, uint8_t *value) {
    uint32_t tmp;
    if (!bitstream_read_bits(buf, 8, &tmp)) return false;
    *value = tmp;
    return true;
}

static inline bool bitstream_read1(bitstream_t *buf, bool *value) {
    uint32_t tmp;
    if (!bitstream_read_bits(buf, 1, &tmp)) return false;
    *value = tmp;
    return true;
}

static bool bitstream_read_ueg(bitstream_t *buf, uint32_t *value) {
    uint32_t bitcount = 0;
    uint32_t tmp;

    for (;;) {
        if (!bitstream_read_bits(buf, 1, &tmp)) return false;
        if (tmp == 0) {
            bitcount++;
        } else {
            // bitOffset--;
            break;
        }
    }

    // bitOffset --;
    uint32_t result = 0;
    if (bitcount) {
        if (!bitstream_read_bits(buf, bitcount, &tmp)) {
            return false;
        }
        result = (uint32_t) ((1 << bitcount) - 1 + tmp);
    }
    *value = result;
    return true;
}

static bool bitstream_read_eg(bitstream_t *buf, int32_t *value) {
    uint32_t tmp;
    if (!bitstream_read_ueg(buf, &tmp)) {
        return false;
    }
    if (tmp & 0x01) {
        *value = (int32_t) (tmp + 1) / 2;
    } else {
        *value = (int32_t) -(tmp / 2);
    }
    return true;
}

static bool bitstream_skip_scaling_list(bitstream_t *buf, uint8_t count) {
    uint32_t lastScale = 8, nextScale = 8;
    int32_t deltaScale;
    for (uint8_t j = 0; j < count; j++) {
        if (nextScale != 0) {
            if (!bitstream_read_eg(buf, &deltaScale)) return false;
            nextScale = (lastScale + deltaScale + 256) % 256;
        }
        lastScale = (nextScale == 0 ? lastScale : nextScale);
    }
    return true;
}

bool sps_parse_dimension_h264(const unsigned char *data, sps_dimension_t *dimension) {
    uint8_t subwc[] = {1, 2, 2, 1};
    uint8_t subhc[] = {1, 2, 1, 1};

    bitstream_t buf;
    bitstream_init(&buf, data);

    uint32_t chroma_format_idc = 1;

    uint32_t width, height;

    bitstream_skip_bits(&buf, 8);
    uint8_t profile_idc;
    bitstream_read8(&buf, &profile_idc);
    // constraint_set[0-5]_flag
    bitstream_skip_bits(&buf, 6);

    /* skip reserved_zero_2bits */
    if (!bitstream_skip_bits(&buf, 2))
        return false;

    // level_idc
    bitstream_skip_bits(&buf, 8);

    uint32_t tmp;
    // id
    bitstream_read_ueg(&buf, &tmp);

    if (profile_idc == 100 || profile_idc == 110 ||
        profile_idc == 122 || profile_idc == 244 || profile_idc == 44 ||
        profile_idc == 83 || profile_idc == 86 || profile_idc == 118 ||
        profile_idc == 128 || profile_idc == 138 || profile_idc == 139 ||
        profile_idc == 134 || profile_idc == 135) {
        if (!bitstream_read_ueg(&buf, &chroma_format_idc)) return false;
        if (chroma_format_idc > 3) return false;
        if (chroma_format_idc == 3) {
            // separate_colour_plane_flag
            bitstream_skip_bits(&buf, 1);
        }

        // bit_depth_luma_minus8
        bitstream_read_ueg(&buf, &tmp);
        // bit_depth_chroma_minus8
        bitstream_read_ueg(&buf, &tmp);
        // qpprime_y_zero_transform_bypass_flag
        bitstream_skip_bits(&buf, 1);

        bool scaling_matrix_present_flag;
        bitstream_read1(&buf, &scaling_matrix_present_flag);
        if (scaling_matrix_present_flag) {
            for (int i = 0; i < ((chroma_format_idc != 3) ? 8 : 12); i++) {
                bool scaling_list_present_flag;
                bitstream_read1(&buf, &scaling_list_present_flag);
                if (scaling_list_present_flag) {
                    bitstream_skip_scaling_list(&buf, i < 6 ? 16 : 64);
                }
            }
        }
    }

    // log2_max_frame_num_minus4
    bitstream_read_ueg(&buf, &tmp);

    uint32_t pic_order_cnt_type;
    bitstream_read_ueg(&buf, &pic_order_cnt_type);
    if (pic_order_cnt_type == 0) {
        // log2_max_pic_order_cnt_lsb_minus4
        bitstream_read_ueg(&buf, &tmp);
    } else if (pic_order_cnt_type == 1) {
        // delta_pic_order_always_zero_flag
        bitstream_skip_bits(&buf, 1);
        // offset_for_non_ref_pic
        bitstream_read_eg(&buf, (int32_t *) (&tmp));
        // offset_for_top_to_bottom_field
        bitstream_read_eg(&buf, (int32_t *) (&tmp));
        uint32_t num_ref_frames_in_pic_order_cnt_cycle;
        bitstream_read_ueg(&buf, &num_ref_frames_in_pic_order_cnt_cycle);

        for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
            // offset_for_ref_frame[i]
            bitstream_read_eg(&buf, (int32_t *) (&tmp));
        }
    }

    // num_ref_frames
    bitstream_read_ueg(&buf, &tmp);
    // gaps_in_frame_num_value_allowed_flag
    bitstream_skip_bits(&buf, 1);
    uint32_t pic_width_in_mbs_minus1;
    if (!bitstream_read_ueg(&buf, &pic_width_in_mbs_minus1)) return false;
    uint32_t pic_height_in_map_units_minus1;
    if (!bitstream_read_ueg(&buf, &pic_height_in_map_units_minus1)) return false;

    bool frame_mbs_only_flag = 0;
    bitstream_read1(&buf, &frame_mbs_only_flag);

    if (!frame_mbs_only_flag) {
        // mb_adaptive_frame_field_flag
        bitstream_skip_bits(&buf, 1);
    }

    // direct_8x8_inference_flag
    bitstream_skip_bits(&buf, 1);
    bool frame_cropping_flag;
    bitstream_read1(&buf, &frame_cropping_flag);
    uint32_t frame_crop_left_offset = 0, frame_crop_right_offset = 0, frame_crop_top_offset = 0,
            frame_crop_bottom_offset = 0;
    if (frame_cropping_flag) {
        bitstream_read_ueg(&buf, &frame_crop_left_offset);
        bitstream_read_ueg(&buf, &frame_crop_right_offset);
        bitstream_read_ueg(&buf, &frame_crop_top_offset);
        bitstream_read_ueg(&buf, &frame_crop_bottom_offset);
    }

    bool vui_parameters_present_flag = false;
    bitstream_read1(&buf, &vui_parameters_present_flag);
    if (vui_parameters_present_flag) {
        if (!skip_vui_parameters(&buf)) return false;
    }

    /* Calculate width and height */
    width = (pic_width_in_mbs_minus1 + 1);
    width *= 16;
    height = (pic_height_in_map_units_minus1 + 1);
    height *= 16 * (2 - frame_mbs_only_flag);
    if (width < 0 || height < 0) {
        return false;
    }

    if (frame_cropping_flag) {
        const uint32_t crop_unit_x = subwc[chroma_format_idc];
        const uint32_t crop_unit_y = subhc[chroma_format_idc] * (2 - frame_mbs_only_flag);

        width -= (frame_crop_left_offset + frame_crop_right_offset) * crop_unit_x;
        height -= (frame_crop_top_offset + frame_crop_bottom_offset) * crop_unit_y;
    }

    dimension->width = width;
    dimension->height = height;
    return true;
}


bool sps_parse_dimension_hevc(const unsigned char *data, sps_dimension_t *dimension) {
    uint8_t subwc[] = {1, 2, 2, 1, 1};
    uint8_t subhc[] = {1, 2, 1, 1, 1};

    uint8_t max_sub_layers_minus1 = 0;

    uint8_t sub_layer_profile_present_flag[6];
    uint8_t sub_layer_level_present_flag[6];
    uint32_t tmp;

    bitstream_t buf;
    bitstream_init(&buf, data);
    // vps_id
    bitstream_skip_bits(&buf, 4);
    bitstream_read_bits(&buf, 3, &tmp);
    max_sub_layers_minus1 = tmp;
    // temporal_id_nesting_flag
    bitstream_skip_bits(&buf, 1);
    {
        parse_profile_info(&buf);

        // level_idc
        bitstream_skip_bits(&buf, 8);
        for (int i = 0; i < max_sub_layers_minus1; i++) {
            uint32_t flg;
            if (!bitstream_read_bits(&buf, 1, &flg)) {
                return false;
            }
            sub_layer_profile_present_flag[i] = flg;
            if (!bitstream_read_bits(&buf, 1, &flg)) {
                return false;
            }
            sub_layer_level_present_flag[i] = flg;
        }

        if (max_sub_layers_minus1 > 0) {
            for (int i = max_sub_layers_minus1; i < 8; i++) {
                // skip 2 bits
                bitstream_skip_bits(&buf, 2);
            }
        }

        for (int i = 0; i < max_sub_layers_minus1; i++) {
            if (sub_layer_profile_present_flag[i]) {
                parse_profile_info(&buf);
            }

            if (sub_layer_level_present_flag[i]) {
                // sub_layer_level_idc[i]
                bitstream_skip_bits(&buf, 8);
            }
        }
    }

    // id
    if (!bitstream_read_ueg(&buf, &tmp)) return false;

    uint32_t chroma_format_idc;
    if (!bitstream_read_ueg(&buf, &chroma_format_idc)) return false;
    if (chroma_format_idc == 3) {
        // separate_colour_plane_flag:1
        bitstream_skip_bits(&buf, 1);
    }

    uint32_t pic_width_in_luma_samples;
    uint32_t pic_height_in_luma_samples;
    if (!bitstream_read_ueg(&buf, &pic_width_in_luma_samples)) return false;
    if (!bitstream_read_ueg(&buf, &pic_height_in_luma_samples)) return false;
    if (pic_width_in_luma_samples <= 0 || pic_height_in_luma_samples <= 0) return false;

    bool conformance_window_flag = false;
    bitstream_read1(&buf, &conformance_window_flag);
    uint32_t conf_win_left_offset = 0, conf_win_right_offset = 0, conf_win_top_offset = 0, conf_win_bottom_offset = 0;
    if (conformance_window_flag) {
        bitstream_read_ueg(&buf, &conf_win_left_offset);
        bitstream_read_ueg(&buf, &conf_win_right_offset);
        bitstream_read_ueg(&buf, &conf_win_top_offset);
        bitstream_read_ueg(&buf, &conf_win_bottom_offset);
    }

    uint32_t width = pic_width_in_luma_samples, height = pic_height_in_luma_samples;
    if (conformance_window_flag) {
        const uint8_t crop_unit_x = subwc[chroma_format_idc];
        const uint8_t crop_unit_y = subhc[chroma_format_idc];
        width -= (conf_win_left_offset + conf_win_right_offset) * crop_unit_x;
        height -= (conf_win_top_offset + conf_win_bottom_offset) * crop_unit_y;
    }
    dimension->width = width;
    dimension->height = height;
    return true;
}

static bool skip_vui_parameters(bitstream_t *buf) {
    bool aspect_ratio_info_present_flag = false;
    bitstream_read1(buf, &aspect_ratio_info_present_flag);
    if (aspect_ratio_info_present_flag) {
        uint8_t aspect_ratio_idc;
        bitstream_read8(buf, &aspect_ratio_idc);
        if (aspect_ratio_idc == EXTENDED_SAR) {
            // sar_width
            bitstream_skip_bits(buf, 16);
            // sar_height
            bitstream_skip_bits(buf, 16);
        }
    }

    bool overscan_info_present_flag = false;
    bitstream_read1(buf, &overscan_info_present_flag);
    if (overscan_info_present_flag) {
        // overscan_appropriate_flag
        bitstream_skip_bits(buf, 1);
    }

    bool video_signal_type_present_flag = false;
    bitstream_read1(buf, &video_signal_type_present_flag);
    if (video_signal_type_present_flag) {
        // video_format
        bitstream_skip_bits(buf, 3);
        // video_full_range_flag
        bitstream_skip_bits(buf, 1);
        bool colour_description_present_flag = false;
        bitstream_read1(buf, &colour_description_present_flag);
        if (colour_description_present_flag) {
            bitstream_skip_bits(buf, 8);
            bitstream_skip_bits(buf, 8);
            bitstream_skip_bits(buf, 8);
        }
    }

    bool chroma_loc_info_present_flag = false;
    bitstream_read1(buf, &chroma_loc_info_present_flag);
    if (chroma_loc_info_present_flag) {
        bitstream_skip_bits(buf, 5);
        bitstream_skip_bits(buf, 5);
    }

    bool timing_info_present_flag = false;
    bitstream_read1(buf, &timing_info_present_flag);
    if (timing_info_present_flag) {
        // num_units_in_tick
        bitstream_skip_bits(buf, 32);
        // time_scale
        bitstream_skip_bits(buf, 32);
        // fixed_frame_rate_flag
        bitstream_skip_bits(buf, 1);
    }

    bool nal_hrd_parameters_present_flag = false;
    bitstream_read1(buf, &nal_hrd_parameters_present_flag);
    if (nal_hrd_parameters_present_flag) {
        if (!skip_hrd_parameters(buf))
            return false;
    }

    bool vcl_hrd_parameters_present_flag = false;
    bitstream_read1(buf, &vcl_hrd_parameters_present_flag);
    if (vcl_hrd_parameters_present_flag) {
        if (!skip_hrd_parameters(buf))
            return false;
    }

    if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
        // low_delay_hrd_flag
        bitstream_skip_bits(buf, 1);
    }

    // pic_struct_present_flag
    bitstream_skip_bits(buf, 1);
    bool bitstream_restriction_flag = false;
    bitstream_read1(buf, &bitstream_restriction_flag);
    if (bitstream_restriction_flag) {
        // motion_vectors_over_pic_boundaries_flag
        bitstream_skip_bits(buf, 1);
        uint32_t tmp;
        // max_bytes_per_pic_denom
        bitstream_read_ueg(buf, &tmp);
        // max_bits_per_mb_denom
        bitstream_read_ueg(buf, &tmp);
        // log2_max_mv_length_horizontal
        bitstream_read_ueg(buf, &tmp);
        // log2_max_mv_length_vertical
        bitstream_read_ueg(buf, &tmp);
        // num_reorder_frames
        bitstream_read_ueg(buf, &tmp);
        // max_dec_frame_buffering
        bitstream_read_ueg(buf, &tmp);
    }

    return true;
}

static bool skip_hrd_parameters(bitstream_t *buf) {
    uint32_t cpb_cnt_minus1;
    bitstream_read_ueg(buf, &cpb_cnt_minus1);
    if (cpb_cnt_minus1 > 31) return false;

    // bit_rate_scale
    bitstream_skip_bits(buf, 4);
    // cpb_size_scale
    bitstream_skip_bits(buf, 4);

    uint32_t tmp;

    for (int sched_sel_idx = 0; sched_sel_idx <= cpb_cnt_minus1; sched_sel_idx++) {
        // bit_rate_value_minus1[sched_sel_idx]
        bitstream_read_ueg(buf, &tmp);
        // cpb_size_value_minus1[sched_sel_idx]
        bitstream_read_ueg(buf, &tmp);
        // cbr_flag[sched_sel_idx]
        bitstream_skip_bits(buf, 1);
    }

    // initial_cpb_removal_delay_length_minus1
    bitstream_skip_bits(buf, 5);
    // cpb_removal_delay_length_minus1
    bitstream_skip_bits(buf, 5);
    // dpb_output_delay_length_minus1
    bitstream_skip_bits(buf, 5);
    // time_offset_length
    bitstream_skip_bits(buf, 5);
    return true;
}

static int parse_profile_info(bitstream_t *buf) {
    // profile_space:2
    bitstream_skip_bits(buf, 2);
    // tier_flag:1
    bitstream_skip_bits(buf, 1);
    // profile_idc:5
    bitstream_skip_bits(buf, 5);

    uint32_t tmp;
    for (int i = 0; i < 32; i++) {
        // profile_compatibility_flag[i]
        bitstream_skip_bits(buf, 1);
    }
    // progressive_source_flag
    bitstream_skip_bits(buf, 1);
    // interlaced_source_flag
    bitstream_skip_bits(buf, 1);
    // non_packed_constraint_flag
    bitstream_skip_bits(buf, 1);
    // frame_only_constraint_flag
    bitstream_skip_bits(buf, 1);

    bitstream_skip_bits(buf, 44);

    return 0;
}
