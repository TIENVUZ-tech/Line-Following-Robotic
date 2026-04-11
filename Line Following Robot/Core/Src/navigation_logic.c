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
static void update_direction_state(float pos);
static void update_last_seen_direction(void);

void Logic_Update(void) {
	//  If the car is stopping: turn of the motors and exit immediately
	if (!g_running) {
		g_state = STATE_STOP;
		motor_control(0, 0);
		return;
	}

	// Running
	switch(g_state) {
		case STATE_TURNING_LEFT:
		case STATE_TURNING_RIGHT:
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
			line_pid.integral = 0;
			line_pid.previous_error = 0;
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
		lost_line_counter = 0;
		update_last_seen_direction();
	} else {
		// The line not found - start counter
		lost_line_counter++;
		if (lost_line_counter >= LOST_LINE_THRESHOLD) {
			g_state = STATE_LOST_LINE;
			motor_control(0, 0); // Short stop before rotating
			return;
		}
	}

	// Calculate position and PID
	float pos = compute_position(); // From -4.0 to 4.0
	float correction = PID_Compute(&line_pid);

	// Choose speed according to deviation
	if (fabs(pos) < THRESH_STRAIGHT) {
		base_speed = SPEED_STRAIGHT;
	}
	else if (fabs(pos) < THRESH_SLIGHT) {
		base_speed = SPEED_SLIGHT;
	}
	else if (fabs(pos) < THRESH_TURN) {
		base_speed = SPEED_TURN;
	}
	else {
		base_speed = SPEED_HARD_TURN;
	}

	pwmL = base_speed + (int)correction;
	pwmR = base_speed - (int)correction;
	motor_control(pwmL, pwmR);

	// Update state
	update_direction_state(pos);
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
		line_pid.integral = 0; // Reset PID integral to avoid windup
		line_pid.previous_error = 0;
		g_state = STATE_FOLLOW_LINE;
		update_last_seen_direction();
		return;
	}

	// Haven't seen the line - turn towards the last lost direction
	if (last_seen_dir == LAST_SEEN_RIGHT) {
		// Line lost to right -> turn right
		pwmL = SPEED_SEARCH;
		pwmR = -SPEED_SEARCH;
	} else {
		// Line lots to left -> turn left
		pwmL = - SPEED_SEARCH;
		pwmR = SPEED_SEARCH;
	}
	motor_control(pwmL, pwmR);
}

// Update g_state according to the current direction
static void update_direction_state(float pos) {
	if (pos > TURNING_THRESHOLD) {
		g_state = STATE_TURNING_RIGHT;
	} else if (pos < - TURNING_THRESHOLD) {
		g_state = STATE_TURNING_LEFT;
	} else {
		g_state = STATE_FOLLOW_LINE;
	}
}

// Update last_seen_dir function
static void update_last_seen_direction(void) {
	// Left eyes (0, 1) are bright but right eyes (3,4) are not -> the car is deviating to the right
	if (sensor_values[0] || sensor_values[1]) {
		last_seen_dir = LAST_SEEN_LEFT;
	}

	// Right eyes (3, 4) are bright but left eyes (0, 1) are not -> the car is deviating to the left
	else if (sensor_values[3] || sensor_values[4]) {
		last_seen_dir = LAST_SEEN_RIGHT;
	}

	else {
		last_seen_dir = LAST_SEEN_CENTER;
	}
}
