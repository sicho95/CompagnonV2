#pragma once
// ============================================================
// ES7210 ADC 4-mic driver — CompagnonV2
// I2C : SDA=GPIO15, SCL=GPIO14  adresse 0x40 (A0=A1=GND)
// I2S RX TDM : MCLK=GPIO42(partagé), SCLK=GPIO9, LRCK=GPIO45
// SDOUT1=GPIO38 (à confirmer sur schéma ADC)
// ============================================================
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Adresses I2C ES7210 (A1:A0)
#define ES7210_ADDR_00  0x40  // A1=GND A0=GND
#define ES7210_ADDR_01  0x41
#define ES7210_ADDR_10  0x42
#define ES7210_ADDR_11  0x43

// Registres
#define ES7210_REG_RESET        0x00
#define ES7210_REG_CLK_CTRL     0x06
#define ES7210_REG_MCLK_DIV     0x07
#define ES7210_REG_OSR          0x09
#define ES7210_REG_SDP_FMT      0x11
#define ES7210_REG_SDP_CTRL     0x12
#define ES7210_REG_MIC12_GAIN   0x43
#define ES7210_REG_MIC34_GAIN   0x44
#define ES7210_REG_MIC1_LP      0x4B
#define ES7210_REG_MIC2_LP      0x4C
#define ES7210_REG_MIC3_LP      0x4D
#define ES7210_REG_MIC4_LP      0x4E
#define ES7210_REG_MIC12_BIAS   0x45
#define ES7210_REG_MIC34_BIAS   0x46
#define ES7210_REG_ANA_CTRL     0x47
#define ES7210_REG_POWER        0x4A

typedef enum {
    ES7210_INPUT_MIC1 = 0,
    ES7210_INPUT_MIC2,
    ES7210_INPUT_MIC3,
    ES7210_INPUT_MIC4,
} es7210_input_mic_t;

typedef enum {
    GAIN_0DB  = 0x00,
    GAIN_3DB  = 0x10,
    GAIN_6DB  = 0x20,
    GAIN_9DB  = 0x30,
    GAIN_12DB = 0x40,
    GAIN_15DB = 0x50,
    GAIN_18DB = 0x60,
    GAIN_21DB = 0x70,
    GAIN_24DB = 0x80,
    GAIN_27DB = 0x90,
    GAIN_30DB = 0xA0,
    GAIN_33DB = 0xB0,
    GAIN_36DB = 0xC0,
} es7210_gain_value_t;

void es7210_adc_init();
void es7210_adc_codec_enable();
void es7210_adc_set_gain(es7210_input_mic_t mic, es7210_gain_value_t gain);

#ifdef __cplusplus
}
#endif
