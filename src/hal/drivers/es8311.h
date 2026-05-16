#pragma once
// ============================================================
// ES8311 Codec driver — CompagnonV2
// I2C : SDA=GPIO15, SCL=GPIO14  (adresse 0x18, ADDR=GND)
// I2S TX : MCLK=GPIO42, SCLK=GPIO9, LRCK=GPIO45, DSDIN=GPIO8
// Sortie différentielle OUTP/OUTN → NS4150B PA_EN=GPIO46
// ============================================================
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ES8311_ADDR         0x18   // ADDR pin = GND

// Registres principaux
#define ES8311_REG_RESET        0x00
#define ES8311_REG_CLK_MGR1     0x01
#define ES8311_REG_CLK_MGR2     0x02
#define ES8311_REG_CLK_MGR3     0x03
#define ES8311_REG_CLK_MGR4     0x04
#define ES8311_REG_CLK_MGR5     0x05
#define ES8311_REG_CLK_MGR6     0x06
#define ES8311_REG_CLK_MGR7     0x07
#define ES8311_REG_CLK_MGR8     0x08
#define ES8311_REG_SDPIN        0x09
#define ES8311_REG_SDPOUT       0x0A
#define ES8311_REG_SYSTEM1      0x0D
#define ES8311_REG_SYSTEM2      0x0E
#define ES8311_REG_SYSTEM3      0x0F
#define ES8311_REG_SYSTEM4      0x10
#define ES8311_REG_SYSTEM5      0x11
#define ES8311_REG_SYSTEM6      0x12
#define ES8311_REG_SYSTEM7      0x13
#define ES8311_REG_SYSTEM8      0x14
#define ES8311_REG_ADC1         0x1C
#define ES8311_REG_ADC2         0x1D
#define ES8311_REG_ADC_EQ       0x1F
#define ES8311_REG_ADC_VOLUME   0x21
#define ES8311_REG_DAC1         0x31
#define ES8311_REG_DAC2         0x32
#define ES8311_REG_GPIO         0x44
#define ES8311_REG_GP           0x45

bool    es8311_init(uint32_t sample_rate);
void    es8311_set_volume(uint8_t vol);
void    es8311_set_mute(bool mute);
void    es8311_set_mic_gain(uint8_t gain);
uint8_t es8311_read_reg(uint8_t reg);

#ifdef __cplusplus
}
#endif
