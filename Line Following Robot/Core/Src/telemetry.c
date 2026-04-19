#include "telemetry.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "string.h"

/* Global variables */
QueueHandle_t xQueueTelemetry = NULL;
uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];
UART_HandleTypeDef *g_huart = NULL;

/* Forward declaration */
extern UART_HandleTypeDef huart1;
extern uint8_t g_running;

/**
 * @brief Task_Telemetry - Low priority task to handle telemetry transmission
 * Receives state from queue and sends via UART1
 */
static void Task_Telemetry(void *pvParameters)
{
    CarState_t state;
    uint8_t tx_buffer[32];
    
    while (1) {
        /* Wait for state from queue with infinite timeout */
        if (xQueueReceive(xQueueTelemetry, &state, portMAX_DELAY) == pdPASS) {
            /* Format and send state string */
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
            
            /* Send via UART1 using DMA */
            uint16_t len = strlen((char *)tx_buffer);
            HAL_UART_Transmit_DMA(g_huart, tx_buffer, len);
        }
    }
}

/**
 * @brief Initialize telemetry system - create queue and task
 */
void Telemetry_Init(UART_HandleTypeDef *huart)
{
    g_huart = huart;
    
    /* Create telemetry queue */
    xQueueTelemetry = xQueueCreate(TELEMETRY_QUEUE_LENGTH, sizeof(CarState_t));
    
    if (xQueueTelemetry == NULL) {
        /* Error - queue creation failed */
        Error_Handler();
    }
    
    /* Create telemetry task with low priority */
    if (xTaskCreate(Task_Telemetry, "Telemetry", 256, NULL, 1, NULL) != pdPASS) {
        /* Error - task creation failed */
        Error_Handler();
    }
    
    /* Enable UART receive interrupt */
    HAL_UART_Receive_IT(huart, uart_rx_buffer, 1);
}

/**
 * @brief Send current state to telemetry queue
 */
void Telemetry_SendState(CarState_t state)
{
    if (xQueueTelemetry != NULL) {
        xQueueSendFromISR(xQueueTelemetry, &state, NULL);
    }
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
            /* Go command */
            g_running = 1;
        } else if (cmd == 'S' || cmd == 's') {
            /* Stop command */
            g_running = 0;
            /* Also stop motors */
            motor_control(0, 0);
        }
        
        /* Re-enable receive interrupt for next byte */
        HAL_UART_Receive_IT(huart, uart_rx_buffer, 1);
    }
}

/* Motor control function declaration - assumed to exist in main.c or navigation_logic.c */
extern void motor_control(int16_t left_speed, int16_t right_speed);
