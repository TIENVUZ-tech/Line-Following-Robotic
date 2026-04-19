#ifndef INC_MOTOR_DRIVER_H_
#define INC_MOTOR_DRIVER_H_

#include "stm32f4xx_hal.h"

// Khai báo ngoại vi để sử dụng Timer 2 (PWM) và UART 1 (Debug) từ main.c
extern TIM_HandleTypeDef htim2;
extern UART_HandleTypeDef huart1;

// Nguyên mẫu hàm
void Motor_Init(void);
void motor_control(int pwmL, int pwmR);
void Debug_Print(float error, int pwm_L, int pwm_R);

#endif /* INC_MOTOR_DRIVER_H_ */
