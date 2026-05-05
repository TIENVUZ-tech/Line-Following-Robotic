#include "telemetry.h"
#include <string.h>
#include <stdio.h>

/* Global variables */
static TelemetryBuffer_t telemetry_buf;
static UART_HandleTypeDef *g_huart = NULL;

uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];

/* Extern declarations to access variables from main.c and motor_driver.c */
extern uint8_t g_running;
extern CarState_t g_state;
extern volatile Odometry_t g_odom;
extern void motor_control(int pwmL, int pwmR);
extern void Encoder_Reset(void);

/**
 * @brief Initialize telemetry system - setup ring buffer
 */
void Telemetry_Init(UART_HandleTypeDef *huart)
{
    g_huart = huart;

    // Initialize ring buffer
    telemetry_buf.head = 0;
    telemetry_buf.tail = 0;
    telemetry_buf.count = 0;
    
    /* Enable UART receive interrupt for 1 byte */
    HAL_UART_Receive_IT(g_huart, uart_rx_buffer, 1);
}

/**
 * @brief Send current state to telemetry buffer (Interrupt safe)
 */
void Telemetry_SendState(CarState_t state)
{
    /* Disable interrupts to safely write to buffer */
    __disable_irq();
    
    if (telemetry_buf.count < TELEMETRY_QUEUE_LENGTH) {
        telemetry_buf.buffer[telemetry_buf.head] = state;
        telemetry_buf.head = (telemetry_buf.head + 1) % TELEMETRY_QUEUE_LENGTH;
        telemetry_buf.count++;
    }
    
    /* Re-enable interrupts */
    __enable_irq();
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

/**
 * @brief Process telemetry buffer - Call this in main while(1) loop
 */
void Telemetry_Process(void)
{
	if (g_huart == NULL || g_huart->gState != HAL_UART_STATE_READY || telemetry_buf.count == 0) return;

	// Get 1 element from buffer
	__disable_irq();
	CarState_t state = telemetry_buf.buffer[telemetry_buf.tail];
	telemetry_buf.tail = (telemetry_buf.tail + 1) % TELEMETRY_QUEUE_LENGTH;
	telemetry_buf.count--;
	__enable_irq();

	// Packet and send
	static uint8_t tx_buf[32];
	const char *name;
	switch(state) {
		case STATE_STOP:
			name = "STOP";
			break;
		case STATE_FOLLOW_LINE:
			name = "FOLLOW_LINE";
			break;
		case STATE_TURNING_LEFT:
			name = "TURN_LEFT";
			break;
		case STATE_TURNING_RIGHT:
			name = "TURN_RIGHT";
			break;
		case STATE_LOST_LINE:
			name = "LOST_LINE";
			break;
		default:
			name = "UNKNOWN";
			break;
	}

	int len = snprintf((char*)tx_buf, sizeof(tx_buf), "$STATE,%s#\r\n", name);
	if (len > 0) {
		HAL_UART_Transmit(g_huart, tx_buf, (uint16_t)len, 30);
	}
}

/**
 * @brief HAL UART Receive Complete Callback
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1) return;

    uint8_t cmd = uart_rx_buffer[0];

    switch (cmd) {
    	case 'G': case 'g':
    		g_running = 1;
    		break;
    	case 'S': case 's':
    		g_running = 0;
    		break;
    	case 'R': case 'r':
    		// Reset position odometry to (0, 0, 0)
    		Encoder_Reset();
    		break;
    	default:
    		break;
    }

    // Reactivate receive interrupt for the next byte
    HAL_UART_Receive_IT(huart, uart_rx_buffer, 1);
}
