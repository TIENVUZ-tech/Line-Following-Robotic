#ifndef INC_TELEMETRY_H_
#define INC_TELEMETRY_H_

#include "stm32f4xx_hal.h"
#include "navigation_logic.h"

<<<<<<< Updated upstream
/* Constants */
#define UART_RX_BUFFER_SIZE 1
=======
/* Telemetry Buffer constants */
#define TELEMETRY_QUEUE_LENGTH 10
#define UART_RX_BUFFER_SIZE 32
>>>>>>> Stashed changes

/* Bare-metal Ring Buffer struct */
typedef struct {
    CarState_t buffer[TELEMETRY_QUEUE_LENGTH];
    uint8_t head;
    uint8_t tail;
    volatile uint8_t count;
} TelemetryBuffer_t;

/* Function prototypes */
void Telemetry_Init(UART_HandleTypeDef *huart);
void Telemetry_SendState(CarState_t state);
void Telemetry_Process(void);

/* External variables */
extern uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];

#endif /* INC_TELEMETRY_H_ */
