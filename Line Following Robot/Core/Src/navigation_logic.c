#include "navigation_logic.h"

// Count the number of consecutive cycles without seeing a line
static int lost_line_counter = 0;

// Remember the direction car lost the line last time to know which way to turn
typedef enum {
	LAST_SEEN_LEFT = -1,
	LAST_SEEN_CENTER = 0,
	LAST_SEEN_RIGHT = 1,
} LastSeenDir_t;

static LastSeenDir_t last_seen_dir = LAST_SEEN_CENTER;

// Static function
static void state_follow_line(void);
static void state_lost_line(void);
static void update_last_seen_direction(void);

void Logic_Update(void) {
	//  If the car is stopping: turn of the motors and exit immediately
	if (!g_running) {
		g_tate = STATE_STOP;
		motor_ctr(0, 0);
		return;
	}

	// Running
	switch(g_state) {
	case STATE_FOLLOW_LINE:
		state_follow_line();
		break;

	case STATE_LOST_LINE:
		state_lost_line();
		break;

	case STATE_STOP:

	default:
		// if g_running = 1, but state is still STOP (just pressed Start), switch to FOLLOW_LINE
		g_state = STATE_FOLLOW_LINE;
		lost_line_counter = 0;
		break;
	}

}

// FOLLOW_LINE
static void state_follow_line(void) {
	// Calculate the total sensors to detect line lost
	int sensor_sum = 0;
	for (int i = 0; i < 5; i++) {
		sensor_sum += sensor_values[i];
	}

	// Update the last line losing direction when still seeing the line
	if (sensor_sum > 0) {
		update_last_seen_direction();
		lost_line_counter = 0;
	} else {
		// The line not found - start counter
		lost_line_counter++;
		if (lost_line_counter >= LOST_LINE_THRESHOLD) {
			g_state = STATE_LOST_LINE;
			motor(0, 0); // Short stop before rotating
			return;
		}
	}

	// Calculate position and PID
	float pos = compute_position(); // From -2.0 to 2.0
	float correction = compute_pid(pos);

	// Choose speed according to deviation
	if (fabs(pos) < 1.0f) basepseed = SPEED_STRAIGHT;
	else if (fabs(pos) <= 2.0f) basespeed = SPEED_SLIGHT_TURN;
	else basespeed = SPEED_HARD_TURN;

	pwmL = basespeed + (int)correction;
	pwmR = basespeed - (int)correction;
	motor_ctr(pwmL, pwmR);

	// Update state
	g_tate = STATE_FOLLOW_LINE;
}

// LOST_LINE
static void state_lost_line(void) {
	// Check again to see if the car can see the line
	int sensor_sum = 0;
	for (int i = 0; i < 5; i++) {
		sensor_sum += sensor_values[i];
	}

	if (sensor_sum > 0) { // Found the line again - turn to FOLLOW_LINE
		lost_line_counter = 0;
		integral = 0; // Reset PID integral to avoid windup
		previous_error = 0;
		g_state = STATE_FOLLOW_LINE;
		return;
	}

	// Haven't seen the line - turn towards the last lost direction
	if (last_seen_dir == LAST_SEEN_RIGHT) {
		// Line lost to right -> turn right
		pwmL = - SPEED_SEARCH;
		pwmR = SPEED_SERARCH;
	} else {
		// Line lots to left -> turn left
		pwmL = SPEED_SEARCH;
		pwmR = - SPEED_SEARCH;
	}
	motor_ctr(pwmL, pwmR);
}

// Update last_seen_dir function
static void update_last_seen_directon(void) {
	// Left eyes (0, 1) are bright but right eyes (3,4) are not -> the car is deviating to the right
	if (sensor_values[0] | sensor_values[1]) {
		last_seen_dir = LAST_SEEN_LEFT;
	}

	// Right eyes (3, 4) are bright but left eyes (0, 1) are not -> the car is deviating to the left
	else if (sensor_values[3] | sensor_values[4]) {
		last_seen_dir = LAST_SEEN_RIGHT;
	}

	else {
		last_seen_dir = LAST_SEEN_CENTER;
	}
}
