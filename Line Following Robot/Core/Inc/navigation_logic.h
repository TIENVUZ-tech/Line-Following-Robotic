#ifndef INC_NAVIGATION_LOGIC_H_
#define INC_NAVIGATION_LOGIC_H_

#include "main.h"
#include "math.h"

// State Machine
typedef enum {
	STATE_STOP = 0, // The car is stationary
	STATE_FOLLOW_LINE = 1, // Follow line
	STATE_LOST_LINE = 2, // Lost line
} CarState_t;

// From sensor.c
extern volatile uint8_t sensor_values[5];
extern volatile float g_error; // positional error

// From pid.c / motor.c
extern float Kp, Ki, Kd;
extern float integral, previous_error;
extern int pwmL, pwmR, basespeed;

extern volatile uint8_t g_running; // 1 = running, 0 = stopping
extern volatile CarsState_t g_state; // Current state

extern float compute_position(void);
extern float compute_pid(float error);
extern motor_ctr(int pwmL, int pwmR);

// Define
#define SPEED_STRAIGHT 55 // Speed when the car goes straight (|error| < 1)
#define SPEED_SLIGHT_TURN 40 // Speed when slightly off (|error| <= 2)
#define SPEED_HARD_TURN 20 // Speed when deviating a lot
#define SPEED_SEARCH 30 // Rotation speed when recalling the line

#define LOST_LINE_THRESHOLD 15 // 15 cycles = 150ms


/**
 * @brief: Main function of the module - called every 10ms from main.c
 * Read sensor status, change state if necessary, calculate PID and control the motors
 */

void Logic_Update(void);

#endif /* INC_NAVIGATION_LOGIC_H_ */
