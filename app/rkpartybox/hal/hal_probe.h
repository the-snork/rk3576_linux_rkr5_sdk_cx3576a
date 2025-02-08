#ifndef _HAL_PROBE_H_
#define _HAL_PROBE_H_
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

// Define constants and enums
#define MAX_REC_PATH 2
#define MAX_MIC_GUITAR 4
#define MAX_CARD_NAME_LEN 12
#define MAX_INPUT_SOURCE SRC_NUM
#define MAX_REC_CHN 8

typedef enum {
    MIC,      // Represents microphone
    GUITAR    // Represents guitar
} mic_guitar_type_t;

// Structures for hardware configuration
typedef struct {
    char card_name[MAX_CARD_NAME_LEN];  // Name of the sound card
    unsigned int sample_rate;           // Sampling rate
    unsigned char channels;              // Number of channels
    rec_channel_mux_t chl_array[MAX_REC_CHN]; // Array of channel configurations
} rec_card_t;

typedef struct {
    char card_name[MAX_CARD_NAME_LEN];  // Name of the speaker sound card
    unsigned int sample_rate;           // Sampling rate
    unsigned int channels;              // Number of channels
    unsigned int bit_width;             // Bit width for audio samples
} spk_t;

typedef struct {
    unsigned char srcs_count;          // Number of input sources
    input_source_t src_order[MAX_INPUT_SOURCE]; // Order of sources
    struct {
        input_source_t input_source;   // Input source type
        char card_name[MAX_CARD_NAME_LEN]; // Corresponding card name
    } sources[MAX_INPUT_SOURCE];
} input_sources_t;

typedef struct {
    unsigned char poor_count;           // Number of poor count
    unsigned char rec_card_count;        // Number of recording cards
    rec_card_t rec_array[MAX_REC_PATH]; // Array of recording cards
    unsigned char mic_guitar_num;        // Number of mic/guitar configurations
    mic_guitar_type_t mic_guitars[MAX_MIC_GUITAR]; // Array of mic/guitar types
} rec_path_t;

typedef struct {
    unsigned char enable;         // Enable flag (1: true, 0: false)
    unsigned short min_adc_value;  // Minimum ADC value
    unsigned short max_adc_value;  // Maximum ADC value
} saradc_key_t;

typedef struct {
    rec_path_t rec_path;        // Recording configuration
    spk_t spk;                  // Speaker configuration
    input_sources_t input_sources; // Input sources configuration
    saradc_key_t saradc_key;    // SAR ADC key configuration
} pbox_hw_config_t;

int parse_json(const char *json_string, pbox_hw_config_t *config);

#ifdef __cplusplus
}
#endif
#endif