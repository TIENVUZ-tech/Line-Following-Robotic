#include "navigation_logic.h"
#include <math.h>

/* Define the loop time based on HAL_Delay(10) in main.c */
#define LOOP_TIME_SEC 0.01f
extern uint8_t g_running;
// Count the number of consecutive cycles without seeing a line
static int lost_line_counter = 0;

static LastSeenDir_t last_seen_dir = LAST_SEEN_CENTER;

// Static functions
static void state_follow_line(void);
static void state_lost_line(void);
static void update_direction_state(float pos);
static void update_last_seen_direction(void);

void Logic_Update(void) {
    // If the car is stopping: turn off the motors and exit immediately
    if (!g_running) {
        g_state = STATE_STOP;
        motor_control(0, 0);
        return;
    }

    // Running State Machine
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

    // 1. Calculate error (position) exactly once (-4.0 to 4.0)
    float error = compute_position();

    // 2. Compute PID correction using the single error value and actual time delta
    float correction = PID_Compute(&line_pid, error, LOOP_TIME_SEC);

    // 3. Choose speed according to the magnitude of the error
    if (fabs(error) < THRESH_STRAIGHT) {
        base_speed = SPEED_STRAIGHT;
    }
    else if (fabs(error) < THRESH_SLIGHT) {
        base_speed = SPEED_SLIGHT;
    }
    else if (fabs(error) < THRESH_TURN) {
        base_speed = SPEED_TURN;
    }
    else {
        base_speed = SPEED_HARD_TURN;
    }

    // 4. Apply motor speeds
    pwmL = base_speed + (int)correction;
    pwmR = base_speed - (int)correction;
    motor_control(pwmL, pwmR);

    // 5. Update state for telemetry
    update_direction_state(error);
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
        // Line lost to left -> turn left
        pwmL = -SPEED_SEARCH;
        pwmR = SPEED_SEARCH;
    }
    motor_control(pwmL, pwmR);
}

// Update g_state according to the current direction
static void update_direction_state(float pos) {
    if (pos > TURNING_THRESHOLD) {
        g_state = STATE_TURNING_RIGHT;
    } else if (pos < -TURNING_THRESHOLD) {
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
