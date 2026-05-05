#include "PID_Logic.h"

void PID_Init(PID_Controller *pid, float Kp, float Ki, float Kd,
              float out_min, float out_max, float dt) {
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;

    pid->out_min = out_min;
    pid->out_max = out_max;

    if (dt > 0.0f) {
        pid->dt = dt;
    } else {
        pid->dt = 0.01f;
    }

    PID_Reset(pid);
}

void PID_Reset(PID_Controller *pid) {
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
}

float PID_Compute(PID_Controller *pid, float setpoint, float measurement) {
    float error = setpoint - measurement;

    float P_out = pid->Kp * error;

    pid->integral += pid->Ki * error * pid->dt;

    if (pid->integral > pid->out_max) {
        pid->integral = pid->out_max;
    } else if (pid->integral < pid->out_min) {
        pid->integral = pid->out_min;
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
