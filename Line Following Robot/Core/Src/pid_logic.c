#include "pid_logic.h"

PID_Controller line_pid;
uint8_t sensor_values[5];
int weights[5] = {-4, -2, 0, 2, 4};
int base_speed = 50;
int pwmL, pwmR;
float current_error = 0;

float derivative = 0;
float dt = 0.005;                   // Time delta (seconds) — adjust to your system
float d = 10.0;

void read_sensors() {
	sensor_values[0] = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
	sensor_values[1] = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);
	sensor_values[2] = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);
	sensor_values[3] = !HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
	sensor_values[4] = !HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_1);
}

void PID_Init(PID_Controller *pid, float Kp, float Ki, float Kd, float max_integral)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->integral = 0.0f;
    pid->previous_error = 0.0f;
    pid->max_integral = max_integral;
}

float PID_Compute(PID_Controller *pid)
{
	int sum = sensor_values[0] + sensor_values[1] + sensor_values[2] + sensor_values[3] + sensor_values[4];

	if (sum > 0) {
	        current_error = (float)((sensor_values[0] * -2) + (sensor_values[1] * -1) +
	                                 (sensor_values[2] * 0)  + (sensor_values[3] * 1) +
	                                 (sensor_values[4] * 2)) / sum;
	}
    // 1. Proportional
    float P = current_error * pid->Kp;

    // 2. Integral with Anti-Windup
    pid->integral += current_error;

    if (pid->integral > pid->max_integral) {
        pid->integral = pid->max_integral;
    } else if (pid->integral < -pid->max_integral) {
        pid->integral = -pid->max_integral;
    }

    // Optional: Reset integral if we cross the center perfectly
    if (current_error == 0.0f) {
        pid->integral = 0.0f;
    }

    float I = pid->integral * pid->Ki;

    // 3. Derivative
    float derivative = current_error - pid->previous_error;
    float D = derivative * pid->Kd;

    // 4. Save error for the next loop
    pid->previous_error = current_error;

    // 5. Return the final turn adjustment
    return (P + I + D);
}

void motor_control(int pwmL, int pwmR) {
	  // 4. Constrain Speeds (0 to 100)
	 if (pwmL > 100) pwmL = 100;
	 if (pwmL < 0) pwmL = 0;

	 if (pwmR > 100) pwmR = 100;
	 if (pwmR < 0) pwmR = 0;

	  // 5. Update Hardware PWM
	  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pwmL);
	  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, pwmR);
}

float compute_position(void) {
	int sensor_sum = 0;
	int weighted_sum = 0;

	for (int i = 0; i < 5; i++) {
		sensor_sum += sensor_values[i];
		weighted_sum += sensor_values[i] * weights[i];
	}

	if (sensor_sum == 0) { // lost line
		return 0.0f;
	}

	return (float)weighted_sum / sensor_sum;
}


