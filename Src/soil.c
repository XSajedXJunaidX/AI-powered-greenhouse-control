#include "soil.h"

// YL-69 in air (dry) reads close to 4095 (12-bit ADC max)
// YL-69 in water (wet) reads close to 0
// We invert so 100% = wet, 0% = dry
#define SOIL_DRY_VAL  3500
#define SOIL_WET_VAL   500

uint8_t Soil_ReadPercent(void) {
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    uint32_t raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    // Clamp to calibration range
    if (raw > SOIL_DRY_VAL) raw = SOIL_DRY_VAL;
    if (raw < SOIL_WET_VAL) raw = SOIL_WET_VAL;

    // Invert and scale to 0-100%
    uint8_t percent = (uint8_t)((SOIL_DRY_VAL - raw) * 100 /
                                (SOIL_DRY_VAL - SOIL_WET_VAL));
    return percent;
}