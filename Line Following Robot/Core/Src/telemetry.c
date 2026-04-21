#include "telemetry.h"
#include <string.h>
#include <stdio.h>

/* Global variables */
uint8_t uart_rx_buffer[1];
UART_HandleTypeDef *g_huart = NULL;

/* Extern declarations to access variables from main.c and motor_driver.c */
extern UART_HandleTypeDef huart1;
extern uint8_t g_running;
extern void motor_control(int pwmL, int pwmR);

/**
 * @brief Initialize telemetry system
 */
void Telemetry_Init(UART_HandleTypeDef *huart)
{
    g_huart = huart;
    /* Enable UART receive interrupt for 1 byte */
    HAL_UART_Receive_IT(g_huart, uart_rx_buffer, 1);
}

/**
 * @brief Send current state via UART Polling
 */
void Telemetry_SendState(CarState_t state)
{
    uint8_t tx_buffer[32];
    memset(tx_buffer, 0, sizeof(tx_buffer));

    switch (state) {
        case STATE_STOP:
            snprintf((char *)tx_buffer, sizeof(tx_buffer), "$STOP#\r\n");
            break;
        case STATE_FOLLOW_LINE:
            snprintf((char *)tx_buffer, sizeof(tx_buffer), "$FOLLOW_LINE#\r\n");
            break;
        case STATE_TURNING_LEFT:
            snprintf((char *)tx_buffer, sizeof(tx_buffer), "$TURN_LEFT#\r\n");
            break;
        case STATE_TURNING_RIGHT:
            snprintf((char *)tx_buffer, sizeof(tx_buffer), "$TURN_RIGHT#\r\n");
            break;
        case STATE_LOST_LINE:
            snprintf((char *)tx_buffer, sizeof(tx_buffer), "$SEARCH_MAZE#\r\n");
            break;
        default:
            snprintf((char *)tx_buffer, sizeof(tx_buffer), "$UNKNOWN#\r\n");
            break;
    }

    /* Send directly via UART (Removed FreeRTOS queue) */
    uint16_t len = strlen((char *)tx_buffer);
    HAL_UART_Transmit(g_huart, tx_buffer, len, 50);
}

/**
 * @brief HAL UART Receive Complete Callback
 * Handle commands from PC: 'G' (Go) and 'S' (Stop)
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        uint8_t cmd = uart_rx_buffer[0];
        
        if (cmd == 'G' || cmd == 'g') {
            g_running = 1;
        } else if (cmd == 'S' || cmd == 's') {
            g_running = 0;
            motor_control(0, 0);
        }
        
        /* Re-enable receive interrupt for the next byte */
        HAL_UART_Receive_IT(huart, uart_rx_buffer, 1);
    }
}
