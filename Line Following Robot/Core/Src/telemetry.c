#include "telemetry.h"
#include <string.h>
#include <stdio.h>

/* Global variables */
TelemetryBuffer_t telemetry_buf;
uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];
UART_HandleTypeDef *g_huart = NULL;

extern uint8_t g_running;
extern void motor_control(int pwmL, int pwmR);

void Telemetry_Init(UART_HandleTypeDef *huart)
{
    g_huart = huart;

    /* Initialize ring buffer */
    telemetry_buf.head = 0;
    telemetry_buf.tail = 0;
    telemetry_buf.count = 0;
    
    /* Enable UART receive interrupt for 1 byte (Command mode) */
    HAL_UART_Receive_IT(g_huart, uart_rx_buffer, 1);
}

void Telemetry_SendState(CarState_t state)
{
    /* Use Critical Section for thread safety */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    
    if (telemetry_buf.count < TELEMETRY_QUEUE_LENGTH) {
        telemetry_buf.buffer[telemetry_buf.head] = state;
        telemetry_buf.head = (telemetry_buf.head + 1) % TELEMETRY_QUEUE_LENGTH;
        telemetry_buf.count++;
    }
    
    if (!primask) __enable_irq();
}

void Telemetry_Process(void)
{
    // Check if UART is ready and there is data to send
    if (g_huart != NULL && g_huart->gState == HAL_UART_STATE_READY && telemetry_buf.count > 0) {

        __disable_irq();
        CarState_t state = telemetry_buf.buffer[telemetry_buf.tail];
        telemetry_buf.tail = (telemetry_buf.tail + 1) % TELEMETRY_QUEUE_LENGTH;
        telemetry_buf.count--;
        __enable_irq();

        // Static buffer so it persists during DMA transfer
        static char tx_buffer[32];

        switch (state) {
            case STATE_STOP:         snprintf(tx_buffer, sizeof(tx_buffer), "$STOP#\r\n"); break;
            case STATE_FOLLOW_LINE:  snprintf(tx_buffer, sizeof(tx_buffer), "$FOLLOW_LINE#\r\n"); break;
            case STATE_TURNING_LEFT: snprintf(tx_buffer, sizeof(tx_buffer), "$TURN_LEFT#\r\n"); break;
            case STATE_TURNING_RIGHT:snprintf(tx_buffer, sizeof(tx_buffer), "$TURN_RIGHT#\r\n"); break;
            case STATE_LOST_LINE:    snprintf(tx_buffer, sizeof(tx_buffer), "$SEARCH_MAZE#\r\n"); break;
            default:                 snprintf(tx_buffer, sizeof(tx_buffer), "$UNKNOWN#\r\n"); break;
        }

        HAL_UART_Transmit_DMA(g_huart, (uint8_t*)tx_buffer, strlen(tx_buffer));
    }
}

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
        
        HAL_UART_Receive_IT(huart, uart_rx_buffer, 1);
    }
}
