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

    float D_out = pid->Kd * (error - pid->prev_error) / pid->dt;

    float total_out = P_out + pid->integral + D_out;

    if (total_out > pid->out_max) {
        total_out = pid->out_max;
    } else if (total_out < pid->out_min) {
        total_out = pid->out_min;
    }

    pid->prev_error = error;

    return total_out;
}
