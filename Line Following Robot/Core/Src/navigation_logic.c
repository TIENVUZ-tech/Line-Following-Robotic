#include "Navigation_logic.h"
#include "PID_Logic.h"
#include "motor_driver.h"
#include "encoder.h"

#define NUM_SENSORS 5

// Motor PWM limits (Assuming 8-bit timer: 0 to 255)
#define MAX_PWM 1000
#define MIN_PWM 0
#define BASE_SPEED 450

extern volatile uint8_t g_running;

static RobotState current_state = STATE_IDLE;
static PID_Controller steering_pid;

uint8_t sensor_array[NUM_SENSORS];

static const float sensor_weights[NUM_SENSORS] = {-4.0f, -2.0f, 0.0f, 2.0f, 4.0f};

static float last_valid_error = 0.0f;

void read_ir_sensors() {
    sensor_array[0] = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) ? 1 : 0;
    sensor_array[1] = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET) ? 1 : 0;
    sensor_array[2] = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_SET) ? 1 : 0;
    sensor_array[3] = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET) ? 1 : 0;
    sensor_array[4] = (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_1) == GPIO_PIN_SET) ? 1 : 0;
}

static bool Calculate_Line_Error(float *out_error) {
    read_ir_sensors();

	float sum_weights = 0.0f;
    int active_sensors = 0;

    for (int i = 0; i < NUM_SENSORS; i++) {
        if (sensor_array[i] == 1) {
            sum_weights += sensor_weights[i];
            active_sensors++;
        }
    }

    if (active_sensors == 0) {
        return false;
    }

    *out_error = sum_weights / active_sensors;
    return true;
}


void LineFollower_Init(void) {
    PID_Init(&steering_pid, 85.0f, 10.0f, 10.0f, -BASE_SPEED, BASE_SPEED, 0.01f);
    Motor_Init();
    Encoder_Init();
    current_state = STATE_IDLE;
    motor_control(0, 0);
}

void LineFollower_SetState(RobotState new_state) {
    if (current_state == new_state) return;

    current_state = new_state;

    switch(current_state) {
        case STATE_IDLE:
        case STATE_STOPPED:
            motor_control(0, 0);
            PID_Reset(&steering_pid);
            g_running = 0;
            break;

        case STATE_FOLLOWING:
            PID_Reset(&steering_pid);
            Encoder_Reset();
            break;

        case STATE_LOST:
            motor_control(0, 0);
            break;
    }
}

RobotState LineFollower_GetState(void) {
    return current_state;
}

void LineFollower_Update(void) {
	if (g_running == 0 && current_state != STATE_IDLE && current_state != STATE_STOPPED) {
	            LineFollower_SetState(STATE_IDLE);
	}

    Encoder_Update();

    float current_error = 0.0f;
    bool line_detected = Calculate_Line_Error(&current_error);

    switch (current_state) {
        case STATE_IDLE:
            if (g_running == 1) {
                LineFollower_SetState(STATE_FOLLOWING);
            }
            break;

        case STATE_FOLLOWING:
        	if (line_detected) {
        		last_valid_error = current_error;
        	} else {

        		if (last_valid_error > 3.1f) {
        			current_error = 5.0f;
        		}

        		else if (last_valid_error < -3.1f) {
        		current_error = -5.0f;
        		}

        		else {
        			current_error = 0.0f;
        		}
        	}

            float correction = PID_Compute(&steering_pid, 0.0f, current_error);

            int pwm_left = BASE_SPEED + (int)correction;
            int pwm_right = BASE_SPEED - (int)correction;

            motor_control(pwm_left, pwm_right);
            break;

        case STATE_LOST:
            // Recovery logic goes here
            break;

        case STATE_STOPPED:
            // Halted safely
            break;
    }
}
