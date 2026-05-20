#include "dht11.h"

// PA1 — adjust if you use a different pin
#define DHT11_PORT  GPIOA
#define DHT11_PIN_N GPIO_PIN_1

static void DHT11_SetOutput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = DHT11_PIN_N;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

static void DHT11_SetInput(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = DHT11_PIN_N;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

// Microsecond delay using DWT cycle counter
// Make sure DWT is enabled in main: CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; DWT->CYCCNT = 0; DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
static void delay_us(uint32_t us) {
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < ticks);
}

DHT11_Data DHT11_Read(void) {
    DHT11_Data result = {0, 0, 1}; // default: error
    uint8_t data[5] = {0};

    // --- Send Start Signal ---
    DHT11_SetOutput();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN_N, GPIO_PIN_RESET);
    HAL_Delay(18);  // Pull low for 18ms
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN_N, GPIO_PIN_SET);
    delay_us(30);

    // --- Wait for DHT11 Response ---
    DHT11_SetInput();
    delay_us(40);

    if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN_N) == GPIO_PIN_SET) {
        return result; // No response
    }
    delay_us(80);
    if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN_N) == GPIO_PIN_RESET) {
        return result; // No response
    }
    delay_us(80);

    // --- Read 40 Bits ---
    for (int i = 0; i < 40; i++) {
        // Wait for pin to go LOW (start of bit)
        uint32_t timeout = 0;
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN_N) == GPIO_PIN_SET) {
            delay_us(1);
            if (++timeout > 100) return result;
        }
        timeout = 0;
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN_N) == GPIO_PIN_RESET) {
            delay_us(1);
            if (++timeout > 100) return result;
        }
        delay_us(40); // Wait 40us: if still HIGH it's a '1', else '0'
        data[i / 8] <<= 1;
        if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN_N) == GPIO_PIN_SET) {
            data[i / 8] |= 1;
        }
    }

    // --- Checksum Validation ---
    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        return result; // Checksum fail
    }

    result.humidity    = data[0];
    result.temperature = data[2];
    result.status      = 0; // OK
    return result;
}