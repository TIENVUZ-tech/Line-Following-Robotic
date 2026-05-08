#ifndef INC_ENCODER_H_
#define INC_ENCODER_H_

#include "stm32f4xx_hal.h"
#include "math.h"

#define ENCODER_CPR 110 // Cycle per round
#define WHEEL_DIAMETER_MM 65.0f // Diameter of the wheel
#define WHEEL_BASE_MM 160.0f // The distance between 2 wheels

#define MM_PER_TICK (M_PI * WHEEL_DIAMETER_MM / ENCODER_CPR)

typedef struct {
	float x; // x coordinate, origin at the starting position
	float y; // y coordinate
	float heading; // direction corner
} Odometry_t;

extern volatile Odometry_t g_odom;

void Encoder_Init(void);
void Encoder_Update(void);
void Encoder_Reset(void);

#endif /* INC_ENCODER_H_ */
