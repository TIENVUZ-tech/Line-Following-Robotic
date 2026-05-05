#include "telemetry.h"
#include <string.h>
#include <stdio.h>

/* Global variables */
<<<<<<< Updated upstream
uint8_t uart_rx_buffer[1];
=======
TelemetryBuffer_t telemetry_buf;
uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];
>>>>>>> Stashed changes
UART_HandleTypeDef *g_huart = NULL;

/* Extern declarations to access variables from main.c and motor_driver.c */
extern UART_HandleTypeDef huart1;
<<<<<<< Updated upstream
extern uint8_t g_running;
extern CarState_t g_state;
extern volatile Odometry_t g_odom;
extern void motor_control(int pwmL, int pwmR);

/**
 * @brief Initialize telemetry system
=======


/**
 * @brief Initialize telemetry system - setup ring buffer
>>>>>>> Stashed changes
 */
void Telemetry_Init(UART_HandleTypeDef *huart)
{
    g_huart = huart;
<<<<<<< Updated upstream
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
=======

    /* Initialize ring buffer */
    telemetry_buf.head = 0;
    telemetry_buf.tail = 0;
    telemetry_buf.count = 0;
    
    /* Enable UART receive interrupt */
    HAL_UART_Receive_IT(huart, uart_rx_buffer, 1);
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

void Telemetry_SendPosition(void) {
	if (g_huart == NULL)  return;
	if (g_huart->gState != HAL_UART_STATE_READY) return;

	static uint8_t pos_buf[64];
	int len = snprintf((char *)pos_buf, sizeof(pos_buf), "$POS,%.1f, %.1f, %.3f#\r\n", g_odom.x, g_odom.y, g_odom.heading);

	if (len >0 && len < (int)sizeof(pos_buf)) {
		HAL_UART_Transmit(g_huart, pos_buf, (uint16_t)len, 30);
	}
}

void Telemetry_Process(void)
{
    /* Check if UART is ready and we have data in the buffer */
    if (g_huart->gState == HAL_UART_STATE_READY && telemetry_buf.count > 0) {

        /* Disable interrupts to safely read from buffer */
        __disable_irq();
        CarState_t state = telemetry_buf.buffer[telemetry_buf.tail];
        telemetry_buf.tail = (telemetry_buf.tail + 1) % TELEMETRY_QUEUE_LENGTH;
        telemetry_buf.count--;
        __enable_irq();

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

        /* Send via UART using DMA */
        uint16_t len = strlen((char *)tx_buffer);
        HAL_UART_Transmit_DMA(g_huart, tx_buffer, len);
>>>>>>> Stashed changes
    }

    /* Send directly via UART (Removed FreeRTOS queue) */
    uint16_t len = strlen((char *)tx_buffer);
    HAL_UART_Transmit(g_huart, tx_buffer, len, 50);
}

/**
 * @brief HAL UART Receive Complete Callback
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
