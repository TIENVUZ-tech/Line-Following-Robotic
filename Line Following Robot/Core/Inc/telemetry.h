#ifndef INC_TELEMETRY_H_
#define INC_TELEMETRY_H_

#include "stm32f4xx_hal.h"
#include "navigation_logic.h"

/* Constants */
#define UART_RX_BUFFER_SIZE 1

/* Function prototypes */
void Telemetry_Init(UART_HandleTypeDef *huart);
void Telemetry_SendState(CarState_t state);

/* External variables */
extern uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];

#endif /* INC_TELEMETRY_H_ */
