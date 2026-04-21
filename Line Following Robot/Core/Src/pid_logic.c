#include "pid_logic.h"

PID_Controller line_pid;
uint8_t sensor_values[5];

// Used strictly for the single position calculation
int weights[5] = {-4, -2, 0, 2, 4};
int base_speed = 50;
int pwmL, pwmR;

void read_sensors() {
    // Pin routing updated for your hardware configuration
    sensor_values[0] = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    sensor_values[1] = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);
    sensor_values[2] = !HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);
    sensor_values[3] = !HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
    sensor_values[4] = !HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_1);
}

void PID_Init(PID_Controller *pid, float Kp, float Ki, float Kd, float max_integral) {
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->integral = 0.0f;
    pid->previous_error = 0.0f;
    pid->max_integral = max_integral;
}

// Calculate position once (-4.0 to 4.0)
float compute_position(void) {
    int sensor_sum = 0;
    int weighted_sum = 0;

    for (int i = 0; i < 5; i++) {
        sensor_sum += sensor_values[i];
        weighted_sum += sensor_values[i] * weights[i];
    }

    if (sensor_sum == 0) {
        return 0.0f; // Lost line logic handled in navigation_logic
    }

    return (float)weighted_sum / sensor_sum;
}

// Pass the calculated error and actual dt into the computation
float PID_Compute(PID_Controller *pid, float error, float dt) {
    // 1. Proportional
    float P = error * pid->Kp;

    // 2. Integral with Anti-Windup and Time Delta
    pid->integral += (error * dt);

    if (pid->integral > pid->max_integral) {
        pid->integral = pid->max_integral;
    } else if (pid->integral < -pid->max_integral) {
        pid->integral = -pid->max_integral;
    }

    if (error == 0.0f) {
        pid->integral = 0.0f;
    }

    float I = pid->integral * pid->Ki;

    // 3. Derivative with Time Delta
    float derivative = (error - pid->previous_error) / dt;
    float D = derivative * pid->Kd;

    // 4. Save error for the next loop
    pid->previous_error = error;

    return (P + I + D);
}
<<<<<<< Updated upstream

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


=======
>>>>>>> Stashed changes
