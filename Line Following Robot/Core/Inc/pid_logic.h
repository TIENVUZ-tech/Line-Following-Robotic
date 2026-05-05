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


void PID_Init(PID_Controller *pid, float Kp, float Ki, float Kd,
              float out_min, float out_max, float dt);

float PID_Compute(PID_Controller *pid, float setpoint, float measurement);

void PID_Reset(PID_Controller *pid);

#endif /* INC_PID_LOGIC_H_ */
