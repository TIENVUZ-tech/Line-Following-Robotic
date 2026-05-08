#include "telemetry.h"
#include <string.h>
#include <stdio.h>

void HC05_Init(HC05_HandleTypeDef *hc05, UART_HandleTypeDef *huart,
               GPIO_TypeDef *en_port, uint16_t en_pin) {

    hc05->huart = huart;
    hc05->en_port = en_port;
    hc05->en_pin = en_pin;

    if (hc05->en_port != NULL) {
        HAL_GPIO_WritePin(hc05->en_port, hc05->en_pin, GPIO_PIN_RESET);
    }

    HC05_SendString(hc05, "Connected!\r\n");
}

void HC05_SendString(HC05_HandleTypeDef *hc05, const char *str) {
    HAL_UART_Transmit(hc05->huart, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
}

void HC05_SendChar(HC05_HandleTypeDef *hc05, char c) {
    HAL_UART_Transmit(hc05->huart, (uint8_t*)&c, 1, HAL_MAX_DELAY);
}

void HC05_SendOdometry(HC05_HandleTypeDef *hc05, const volatile Odometry_t *odom) {
    char buf[48];
    /* Convert mm → m with 3 decimal places (1 mm resolution).
       Use absolute value + sign string so "-0.050 m" prints correctly
       (integer division of -50/1000 = 0, losing the sign without this). */
    int32_t  x_mm = (int32_t)odom->x;
    int32_t  y_mm = (int32_t)odom->y;
    int32_t  hmr  = (int32_t)(odom->heading * 1000.0f);

    uint32_t x_abs = (uint32_t)(x_mm < 0 ? -x_mm : x_mm);
    uint32_t y_abs = (uint32_t)(y_mm < 0 ? -y_mm : y_mm);

    int len = snprintf(buf, sizeof(buf), "X:%s%lu.%03lu Y:%s%lu.%03lu H:%ld\r\n",
                       (x_mm < 0 ? "-" : ""), (unsigned long)(x_abs / 1000), (unsigned long)(x_abs % 1000),
                       (y_mm < 0 ? "-" : ""), (unsigned long)(y_abs / 1000), (unsigned long)(y_abs % 1000),
                       (long)hmr);
    HAL_UART_Transmit(hc05->huart, (uint8_t*)buf, (uint16_t)len, HAL_MAX_DELAY);
}
