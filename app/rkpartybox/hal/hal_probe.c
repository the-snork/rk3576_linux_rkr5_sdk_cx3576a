#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "cJSON.h"
#include "os_minor_type.h"
#include "utils_common.h"
#include "hal_partybox.h"
#include "hal_probe.h"

// Function to parse mic/guitar type from a string
mic_guitar_type_t parse_mic_guitar_type(const char *str) {
    if (strcasecmp(str, CSTR(MIC)) == 0) return MIC;
    if (strcasecmp(str, CSTR(GUITAR)) == 0) return GUITAR;
    fprintf(stderr, "Error: Unknown mic/guitar type: %s\n", str);
    assert(0);
}

// Function to parse input source type from a string
input_source_t parse_input_source(const char *str) {
    if (strcasecmp(str, CSTR(SRC_CHIP_USB)) == 0) return SRC_CHIP_USB;
    if (strcasecmp(str, CSTR(SRC_CHIP_BT)) == 0) return SRC_CHIP_BT;
    if (strcasecmp(str, CSTR(SRC_CHIP_UAC)) == 0) return SRC_CHIP_UAC;
    if (strcasecmp(str, CSTR(SRC_EXT_BT)) == 0) return SRC_EXT_BT;
    if (strcasecmp(str, CSTR(SRC_EXT_USB)) == 0) return SRC_EXT_USB;
    if (strcasecmp(str, CSTR(SRC_EXT_AUX)) == 0) return SRC_EXT_AUX;
    if (strcasecmp(str, CSTR(SRC_CHIP_SPDIF)) == 0) return SRC_CHIP_SPDIF;
    fprintf(stderr, "Error: Unknown input source: %s\n", str);
    assert(0);
}

// Function to parse recording channel type from a string
rec_channel_mux_t parse_rec_channel(const char *str) {
    if (strcasecmp(str, CSTR(REC_CHL_MIC)) == 0) return REC_CHL_MIC;
    if (strcasecmp(str, CSTR(REC_CHL_GUITAR)) == 0) return REC_CHL_GUITAR;
    if (strcasecmp(str, CSTR(REC_CHL_SCENE)) == 0) return REC_CHL_SCENE;
    if (strcasecmp(str, CSTR(REC_CHL_REF)) == 0) return REC_CHL_REF;
    if (strcasecmp(str, CSTR(REC_CHL_REF_HOLD)) == 0) return REC_CHL_REF_HOLD;
    if (strcasecmp(str, CSTR(REC_CHL_PLACE_HOLD)) == 0) return REC_CHL_PLACE_HOLD;
    fprintf(stderr, "Error: Unknown recording channel: %s\n", str);
    assert(0);
}

// Function to parse JSON and populate the structure
int parse_json(const char *json_string, pbox_hw_config_t *config) {
    cJSON *root = cJSON_Parse(json_string);
    if (!root) {
        fprintf(stderr, "Error parsing JSON.\n");
        assert(0);
    }

    cJSON * pbox_hw_config = cJSON_GetObjectItem(root, "pbox_hw_config");
    if (!pbox_hw_config) {
        fprintf(stderr, "Error parsing pbox_hw_config.\n");
        assert(0);
    }

    // Parse `rec_path`
    cJSON *rec_path = cJSON_GetObjectItem(pbox_hw_config, "rec_path");
    cJSON *rec_array = cJSON_GetObjectItem(rec_path, "rec_array");

    config->rec_path.poor_count = cJSON_GetObjectItem(rec_path, "poor_count")->valueint;
    config->rec_path.rec_card_count = cJSON_GetArraySize(rec_array);
    for (int i = 0; i < config->rec_path.rec_card_count; i++) {
        cJSON *rec_card = cJSON_GetArrayItem(rec_array, i);
        strcpy(config->rec_path.rec_array[i].card_name, cJSON_GetObjectItem(rec_card, "card_name")->valuestring);
        config->rec_path.rec_array[i].sample_rate = cJSON_GetObjectItem(rec_card, "sample_rate")->valueint;
        config->rec_path.rec_array[i].channels = cJSON_GetObjectItem(rec_card, "channels")->valueint;

        cJSON *chl_array = cJSON_GetObjectItem(rec_card, "chl_array");
        for (int j = 0; j < MAX_REC_CHN; ++j) {
            cJSON *chl = cJSON_GetArrayItem(chl_array, j);
            if (!chl) break;
            config->rec_path.rec_array[i].chl_array[j] = parse_rec_channel(chl->valuestring);
        }
    }

    cJSON *mic_guitars = cJSON_GetObjectItem(rec_path, "mic_guitars");
    config->rec_path.mic_guitar_num = cJSON_GetArraySize(mic_guitars);
    for (int i = 0; i < config->rec_path.mic_guitar_num; i++) {
        config->rec_path.mic_guitars[i] = parse_mic_guitar_type(cJSON_GetArrayItem(mic_guitars, i)->valuestring);
    }

    // Parse `spk`
    cJSON *spk = cJSON_GetObjectItem(pbox_hw_config, "spk");
    strncpy(config->spk.card_name, cJSON_GetObjectItem(spk, "card_name")->valuestring, MAX_CARD_NAME_LEN-1);
    config->spk.sample_rate = cJSON_GetObjectItem(spk, "sample_rate")->valueint;
    config->spk.channels = cJSON_GetObjectItem(spk, "channels")->valueint;
    config->spk.bit_width = cJSON_GetObjectItem(spk, "bit_width")->valueint;

    // Parse `input_sources`
    cJSON *input_sources = cJSON_GetObjectItem(pbox_hw_config, "input_sources");

    cJSON *src_order = cJSON_GetObjectItem(input_sources, "srcs_with_order");
    config->input_sources.srcs_count = cJSON_GetArraySize(src_order);

    for (int i = 0; i < config->input_sources.srcs_count; i++) {
        config->input_sources.src_order[i] = parse_input_source(cJSON_GetArrayItem(src_order, i)->valuestring);
    }

    cJSON *sources_info = cJSON_GetObjectItem(input_sources, "sources_info");
    if (!sources_info || !cJSON_IsArray(sources_info)) {
        fprintf(stderr, "Error: 'sources_info' is missing or not an array\n");
        assert(0);
    }

    for (int i = 0; i < config->input_sources.srcs_count; i++) {
        cJSON *source = cJSON_GetArrayItem(sources_info, i);
        if (!source || !cJSON_IsObject(source)) {
            fprintf(stderr, "Error: 'sources_info[%d]' is missing or not an object\n", i);
            assert(0);
        }

        cJSON *input_source_obj = cJSON_GetObjectItem(source, "source");
        cJSON *card_name_obj = cJSON_GetObjectItem(source, "card");
        if (!input_source_obj || !cJSON_IsString(input_source_obj)) {
            fprintf(stderr, "Error: 'sources_info[%d].source' is missing or not a string\n", i);
            assert(0);
        }

        if (card_name_obj && !cJSON_IsString(card_name_obj) && !cJSON_IsNull(card_name_obj)) {
            fprintf(stderr, "Error: 'sources_info[%d].card' is neither a string nor null\n", i);
            assert(0);
        }

        // Parse the input source
        config->input_sources.sources[i].input_source = parse_input_source(input_source_obj->valuestring);
        // Copy card name (handle null case)
        if (card_name_obj && cJSON_IsString(card_name_obj)) {
            strncpy(config->input_sources.sources[i].card_name, card_name_obj->valuestring, MAX_CARD_NAME_LEN - 1);
            config->input_sources.sources[i].card_name[MAX_CARD_NAME_LEN - 1] = '\0'; // Ensure null termination
        } else {
            config->input_sources.sources[i].card_name[0] = '\0'; // Empty string for null card
        }
    }

    // Parse `saradc_key`
    cJSON *saradc_key = cJSON_GetObjectItem(pbox_hw_config, "saradc_key");
    config->saradc_key.enable = cJSON_IsTrue(cJSON_GetObjectItem(saradc_key, "enable"));
    config->saradc_key.min_adc_value = cJSON_GetObjectItem(saradc_key, "min_adc_value")->valueint;
    config->saradc_key.max_adc_value = cJSON_GetObjectItem(saradc_key, "max_adc_value")->valueint;

    cJSON_Delete(root);
    return 0;
}
