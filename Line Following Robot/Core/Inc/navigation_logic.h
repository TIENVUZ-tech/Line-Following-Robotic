#ifndef INC_NAVIGATION_LOGIC_H_
#define INC_NAVIGATION_LOGIC_H_

#include "main.h"
#include "math.h"

extern uint8_t sensor_values[5];
extern PID_Controller line_pid;

extern uint8_t g_running; // 1 = running, 0 = stopping
extern CarState_t g_state; // Current state

extern int base_speed;
extern int pwmL, pwmR;

extern float compute_position(void);
extern float PID_Compute(PID_Controller *pid);
extern void motor_control(int pwmL, int pwmR);

// Define speed
#define SPEED_STRAIGHT 80 // Speed when the car goes straight (|error| < 1)
#define SPEED_SLIGHT 65 // Speed when slightly off (|error| <= 2)
#define SPEED_TURN 45 // Speed when turn left or turn right
#define SPEED_HARD_TURN 25 // Speed when the car faces hard turn
#define SPEED_SEARCH 30 // Rotation speed when recalling the line

// Error threshold to classify speed levels
#define THRESH_STRAIGHT 1.0f
#define THRESH_SLIGHT 1.5f
#define THRESH_TURN 3.0f

// Error threshold to update g_state to TURNING_LEFT/RIGHT
#define TURNING_THRESHOLD 1.2f

#define LOST_LINE_THRESHOLD 15 // 15 cycles = 150ms


/**
 * @brief: Main function of the module - called every 10ms from main.c
 * Read sensor status, change state if necessary, calculate PID and control the motors
 */

void Logic_Update(void);

#endif /* INC_NAVIGATION_LOGIC_H_ */
