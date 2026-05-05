/*
 * Navigation_logic.h
 *
 *  Created on: May 2, 2026
 *      Author: DELL
 */

#ifndef INC_NAVIGATION_LOGIC_H_
#define INC_NAVIGATION_LOGIC_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    STATE_IDLE,
    STATE_FOLLOWING,
    STATE_LOST,
    STATE_STOPPED
} RobotState;


void LineFollower_Init(void);


void LineFollower_Update(void);


void LineFollower_SetState(RobotState new_state);

RobotState LineFollower_GetState(void);

#endif /* INC_NAVIGATION_LOGIC_H_ */
