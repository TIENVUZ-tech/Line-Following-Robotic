#ifndef INC_MOTOR_DRIVER_H_
#define INC_MOTOR_DRIVER_H_

#include "stm32f4xx_hal.h"

extern TIM_HandleTypeDef htim2;

void Motor_Init(void);
void motor_control(int pwmL, int pwmR);


#endif /* INC_MOTOR_DRIVER_H_ */
