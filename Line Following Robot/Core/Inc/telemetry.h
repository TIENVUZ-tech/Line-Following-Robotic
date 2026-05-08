#ifndef INC_TELEMETRY_H_
#define INC_TELEMETRY_H_

#include "stm32f4xx_hal.h"
#include "encoder.h"

typedef struct {
    UART_HandleTypeDef *huart;
    GPIO_TypeDef *en_port;
    uint16_t en_pin;
} HC05_HandleTypeDef;

void HC05_Init(HC05_HandleTypeDef *hc05, UART_HandleTypeDef *huart,
               GPIO_TypeDef *en_port, uint16_t en_pin);

void HC05_SendString(HC05_HandleTypeDef *hc05, const char *str);
void HC05_SendChar(HC05_HandleTypeDef *hc05, char c);
void HC05_SendOdometry(HC05_HandleTypeDef *hc05, const volatile Odometry_t *odom);

#endif /* INC_TELEMETRY_H_ */
