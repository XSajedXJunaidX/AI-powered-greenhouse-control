#include "bluetooth.h"
#include <stdio.h>
#include <string.h>

void Bluetooth_Init(void) {
    // USART1 already initialized by CubeMX generated MX_USART1_UART_Init()
    // Nothing extra needed
}

void Bluetooth_SendData(uint8_t temperature, uint8_t humidity) {
    char buf[64];
    // JSON format — easy to parse in Python
    snprintf(buf, sizeof(buf), "{\"temp\":%d,\"humidity\":%d}\n",
             temperature, humidity);
    HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
}