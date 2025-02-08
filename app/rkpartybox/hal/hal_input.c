
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include "hal_partybox.h"
#include "utils_common.h"
#include "os_minor_type.h"
#include "pbox_interface.h"
#include "hal_probe.h"

typedef struct {
    int kernel_space;
    int user_space;
} key_pair_t;

static const key_pair_t KEY_TABLE[] = {
    /*kernel    user*/
    {373,       HKEY_MODE}, //KEY_MODE
    {207,       HKEY_PLAY}, //KEY_PLAY
    {209,       HKEY_GPIO_BOOST},
    {115,       HKEY_VOLUP}, //KEY_VOLUMEUP
    {114,       HKEY_VOLDOWN}, //HKEY_VOLDOWN
    {248,       HKEY_MIC1MUTE}, //KEY_MICMUTE
    {0x21e,     HKEY_GPIO_LIGHTS},
};

static pbox_hw_config_t audio_hw_config = {0};

char* getInputSourceString(input_source_t source) {
    switch (source) {
        case SRC_CHIP_USB: return "usb";
        case SRC_CHIP_BT: return "bt";
        case SRC_CHIP_UAC: return "uac";
        case SRC_CHIP_SPDIF: return "spdif";
        case SRC_EXT_BT: return "ext_bt";
        case SRC_EXT_USB: return "ext_usb";
        case SRC_EXT_AUX: return "ext_aux";
        default: return "unkown";
    }
}

int hal_key_convert_kernel_to_upper(uint32_t value) {
    for (int i = 0; i < sizeof(KEY_TABLE)/sizeof(key_pair_t); i++) {
        if(value == KEY_TABLE[i].kernel_space)
            return KEY_TABLE[i].user_space;
    }

    return HKEY_IDLE;
}

//this is speaker or codec
char *hal_get_spk_codec_card(void) {
    return audio_hw_config.spk.card_name;
}

uint8_t hal_get_audio_spk_codec_channel(void) {
    return audio_hw_config.spk.channels;
}

uint32_t hal_get_audio_spk_codec_sample_rate(void) {
    return audio_hw_config.spk.sample_rate;
}

uint8_t hal_get_audio_spk_codec_bit_width(void) {
    return audio_hw_config.spk.bit_width;
}

//this is mic, guitar, ref, scene...
uint8_t hal_get_record_cards_num(void) {
    return audio_hw_config.rec_path.rec_card_count;
}

char *hal_get_record_card_name(int i) {
    assert(i < audio_hw_config.rec_path.rec_card_count);
    return audio_hw_config.rec_path.rec_array[i].card_name;
}

uint32_t hal_get_record_card_sample_rate(int i) {
    assert(i < audio_hw_config.rec_path.rec_card_count);
    return audio_hw_config.rec_path.rec_array[i].sample_rate;
}

uint8_t hal_get_record_card_channels(int i) {
    assert(i < audio_hw_config.rec_path.rec_card_count);
    return audio_hw_config.rec_path.rec_array[i].channels;
}

const rec_channel_mux_t* hal_get_record_card_chn_array(int i) {
    assert(i < audio_hw_config.rec_path.rec_card_count);
    return &audio_hw_config.rec_path.rec_array[i].chl_array[0];
}

uint32_t hal_get_record_card_chn_mux(int i) {
    uint32_t mux = 0;

    assert(i < audio_hw_config.rec_path.rec_card_count);
    for (int j = 0; j < hal_get_record_card_channels(i); j++) {
        mux |= audio_hw_config.rec_path.rec_array[i].chl_array[j];
    }

    return mux;
}

uint8_t hal_get_kalaok_poor_count(void) {
    return audio_hw_config.rec_path.poor_count;
}

input_source_t hal_get_favor_source_order(int index) {
    assert(index < audio_hw_config.input_sources.srcs_count);
    return audio_hw_config.input_sources.src_order[index];
}

uint8_t hal_get_supported_source_numbers(void) {
    return audio_hw_config.input_sources.srcs_count;
}

char *hal_get_audio_card(input_source_t source) {
    bool found = false;
    for (int i = 0; i < hal_get_supported_source_numbers(); i++) {
        if (audio_hw_config.input_sources.src_order[i] == source) {
            found = true;
            break;
        }
    }

    assert(found == true);
    for (int i = 0; i < hal_get_supported_source_numbers(); i++) {
        if (source == audio_hw_config.input_sources.sources[i].input_source)
            return audio_hw_config.input_sources.sources[i].card_name;
    }

    return NULL;
}

bool hal_get_sara_adc_usage(void) {
    return audio_hw_config.saradc_key.enable;
}

uint16_t hal_max_saradc_val(void) {
    return audio_hw_config.saradc_key.max_adc_value;
}

uint16_t hal_min_saradc_val(void) {
    return audio_hw_config.saradc_key.min_adc_value;
}

uint8_t hal_get_mic_guitar_num(void) {
    return audio_hw_config.rec_path.mic_guitar_num;
}

uint8_t hal_get_mic_num(void) {
    uint32_t num = 0;

    //for mic and guitar, must place mics ahead, guitar last.
    for (int i = 0; i < hal_get_mic_guitar_num(); i++) {
        if(audio_hw_config.rec_path.mic_guitars[i] == MIC)
            num++;
        else
            break;
    }

    return num;
}

uint8_t hal_get_mic_guitar_value(uint8_t index) {
    assert(index < hal_get_mic_guitar_num());
    return audio_hw_config.rec_path.mic_guitars[index];
}

int hal_probe_pbox_audio_config(const char *filename) {
    char *json = cJSON_file_read(filename);

    parse_json(json, &audio_hw_config);
    os_free(json);

    return 0;
}