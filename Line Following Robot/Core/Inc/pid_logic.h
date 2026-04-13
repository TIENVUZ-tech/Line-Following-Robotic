#ifndef INC_PID_LOGIC_H_
#define INC_PID_LOGIC_H_

#include "stm32f4xx_hal.h"

typedef struct {
    float Kp;
    float Ki;
    float Kd;
    float integral;
    float previous_error;
    float max_integral; // For anti-windup
} PID_Controller;

extern PID_Controller line_pid;
extern uint8_t sensor_values[5];
extern int base_speed;
extern int pwmL, pwmR;
extern float current_error;

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern TIM_HandleTypeDef htim2;
extern UART_HandleTypeDef huart1;


// Functions
void read_sensors(void);
float PID_Compute(PID_Controller *pid);
void PID_Init(PID_Controller *pid, float Kp, float Ki, float Kd, float max_integral);
void motor_control(int pwmL, int pwmR);
float compute_position(void);

#endif /* INC_PID_LOGIC_H_ */
