#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

extern UART_HandleTypeDef huart1;

void Bluetooth_Init(void);
void Bluetooth_SendData(uint8_t temperature, uint8_t humidity);

#endif