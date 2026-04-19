#ifndef INC_NAVIGATION_LOGIC_H_
#define INC_NAVIGATION_LOGIC_H_

#include "stm32f4xx.h"
#include "math.h"
#include "pid_logic.h"
#include "motor_driver.h"

// State Machine
typedef enum {
	STATE_STOP = 0, // The car is stationary
	STATE_FOLLOW_LINE = 1, // Follow line
	STATE_TURNING_LEFT = 2, // turn left
	STATE_TURNING_RIGHT = 3, // turn right
	STATE_LOST_LINE = 4, // Lost line
} CarState_t;

// Remember the direction car lost the line last time to know which way to turn
typedef enum {
	LAST_SEEN_LEFT = -1,
	LAST_SEEN_CENTER = 0,
	LAST_SEEN_RIGHT = 1,
} LastSeenDir_t;

extern uint8_t g_running; // 1 = running, 0 = stopping
extern CarState_t g_state; // Current state

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
