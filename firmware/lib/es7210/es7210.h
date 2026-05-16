#pragma once
// ============================================================
// ES7210 — stub minimal pour compilation
// Le Waveshare ESP32-S3-AMOLED-2.16" n'embarque pas d'ES7210 ;
// ce stub permet la compilation. Les appels réels sont no-ops.
// ============================================================
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ES7210_INPUT_MIC1 = 0,
    ES7210_INPUT_MIC2,
    ES7210_INPUT_MIC3,
    ES7210_INPUT_MIC4,
} es7210_input_mic_t;

typedef enum {
    GAIN_0DB  = 0,
    GAIN_3DB,
    GAIN_6DB,
    GAIN_9DB,
    GAIN_12DB,
    GAIN_15DB,
    GAIN_18DB,
    GAIN_21DB,
    GAIN_24DB,
    GAIN_27DB,
    GAIN_30DB,
    GAIN_33DB,
    GAIN_36DB,
} es7210_gain_value_t;

static inline void es7210_adc_init()                                         {}
static inline void es7210_adc_codec_enable()                                 {}
static inline void es7210_adc_set_gain(es7210_input_mic_t m, es7210_gain_value_t g) { (void)m; (void)g; }

#ifdef __cplusplus
}
#endif
