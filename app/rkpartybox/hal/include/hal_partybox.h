#ifndef _PTBOX_HARDWARE_BOARD_H_
#define _PTBOX_HARDWARE_BOARD_H_
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define DSP_MAIN_MAX_VOL        32
#define DSP_MUSIC_MAX_VOL       32
#define DSP_MIC_REVERB_MAX_VOL  32
#define DSP_MIC_TREBLE_MAX_VOL  32
#define DSP_MIC_BASS_MAX_VOL    32
#define DSP_MIC_MAX_VOL         32

typedef enum {
    SRC_CHIP_USB,
    SRC_CHIP_BT,
    SRC_CHIP_UAC,
    SRC_CHIP_SPDIF,
    SRC_EXT_BT,
    SRC_EXT_USB,
    SRC_EXT_AUX,
    SRC_NUM
} input_source_t;

typedef enum {
    REC_CHL_MIC         = 1 << 0,
    REC_CHL_GUITAR      = 1 << 1,
    REC_CHL_SCENE       = 1 << 2,
    REC_CHL_REF         = 1 << 3,
    REC_CHL_REF_HOLD    = 1 << 4,
    REC_CHL_PLACE_HOLD  = 1 << 5
} rec_channel_mux_t;

typedef enum {
    HKEY_IDLE,
    HKEY_PLAY,//KEY APP PLAY
    HKEY_VOLUP,
    HKEY_VOLDOWN,
    HKEY_MODE,
    HKEY_MIC1MUTE,
    HKEY_MIC2MUTE,
    HKEY_MIC1BASS,
    HKEY_MIC2BASS,
    HKEY_MIC1TREB,
    HKEY_MIC2TREB,
    HKEY_MIC1REVB,
    HKEY_MIC2REVB,
    HKEY_MIC1_VOL,
    HKEY_MIC2_VOL,
    HKEY_GPIO_BOOST,
    HKEY_GPIO_LIGHTS,
    HKEY_ROTARY_POS,
    HKEY_ROTARY_NEG,
    HKEY_NUM
} hal_key_t;

float hw_main_gain(uint8_t index);
float hw_music_gain(uint8_t index);
float hw_mic_reverb(uint8_t index);
float hw_mic_treble(uint8_t index);
float hw_mic_bass(uint8_t index);
float hw_mic_gain(uint8_t index);
float hw_guitar_reverb(uint8_t index);
float hw_guitar_treble(uint8_t index);
float hw_guitar_bass(uint8_t index);

const float* hw_get_main_volume_table(uint16_t* size);
const float* hw_get_music_volume_table(uint16_t* size);
const float* hw_get_mic_reverb_table(uint16_t* size);
const float* hw_get_mic_treble_table(uint16_t* size);
const float* hw_get_mic_bass_table(uint16_t* size);
const float* hw_get_guitar_reverb_table(uint16_t* size);
const float* hw_get_guitar_treble_table(uint16_t* size);
const float* hw_get_guitar_bass_table(uint16_t* size);

int hal_dsp_max_main_vol(void);
int hal_dsp_max_music_vol(void);
int hal_dsp_max_mic_reverb(void);
int hal_dsp_max_mic_treble(void);
int hal_dsp_max_mic_bass(void);
int hal_dsp_max_mic_vol(void);

char *hal_get_audio_card(input_source_t source);
char* getInputSourceString(input_source_t source);
int hal_key_convert_kernel_to_upper(uint32_t value);

//speaker or codec
char *hal_get_spk_codec_card(void);
uint8_t hal_get_audio_spk_codec_channel(void);
uint32_t hal_get_audio_spk_codec_sample_rate(void);
uint8_t hal_get_audio_spk_codec_bit_width(void);

uint8_t hal_get_record_cards_num(void);
char *hal_get_record_card_name(int i);
uint32_t hal_get_record_card_sample_rate(int i);
uint8_t hal_get_record_card_channels(int i);
const rec_channel_mux_t* hal_get_record_card_chn_array(int i);
uint32_t hal_get_record_card_chn_mux(int i);
//guitar
char *hal_get_kalaok_guitar_card(void);
//mic
char *hal_get_kalaok_mic_card(void);
uint8_t hal_get_kalaok_mic_ref_layout(void);
uint8_t hal_get_kalaok_mic_rec_layout(void);
uint8_t hal_get_kalaok_mic_chn_layout(void);
uint8_t hal_get_kalaok_mic_rec_channel(void);
uint8_t hal_get_mic_guitar_value(uint8_t index);
uint8_t hal_get_kalaok_poor_count(void);
uint8_t hal_get_kalaok_ref_hard_mode(void);
uint32_t hal_get_kalaok_rec_sample_rate(void);

//scene
char *hal_get_audio_scene_card(void);
uint8_t hal_get_scene_mic_ref_layout(void);
uint8_t hal_get_scene_mic_rec_layout(void);
uint8_t hal_get_scene_mic_rec_channel(void);
uint8_t hal_get_scene_ref_hard_mode(void);
uint32_t hal_get_scene_rec_sample_rate(void);

//input source
input_source_t hal_get_favor_source_order(int index);
uint8_t hal_get_supported_source_numbers(void);

bool hal_get_sara_adc_usage(void);
uint16_t hal_max_saradc_val(void);
uint16_t hal_min_saradc_val(void);

uint8_t hal_get_mic_guitar_num(void);
uint8_t hal_get_mic_num(void);

int hal_probe_pbox_audio_config(const char *filename);
#ifdef __cplusplus
}
#endif
#endif
