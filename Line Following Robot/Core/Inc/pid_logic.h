/*
 * PID_Logic.h
 *
 *  Created on: May 2, 2026
 *      Author: DELL
 */

#ifndef INC_PID_LOGIC_H_
#define INC_PID_LOGIC_H_

typedef struct {
    float Kp;
    float Ki;
    float Kd;

    float out_min;
    float out_max;

    float dt;

    float integral;
    float prev_error;
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
float PID_Compute(PID_Controller *pid, float error, float dt);
void PID_Init(PID_Controller *pid, float Kp, float Ki, float Kd, float max_integral);
float compute_position(void);

#endif /* INC_PID_LOGIC_H_ */
