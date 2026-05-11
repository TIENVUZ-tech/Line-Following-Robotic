#include "encoder.h"

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

volatile Odometry_t g_odom = {0.0f, 0.0f, 0.0f};

static int prev_left = 0;
static int prev_right = 0;

void Encoder_Init(void) {
	HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
	HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);

	__HAL_TIM_SET_COUNTER(&htim3, 32768);
	__HAL_TIM_SET_COUNTER(&htim4, 32768);

	prev_left = 32768;
	prev_right = 32768;
}

void Encoder_Update(void) {
	int cur_left = (int)__HAL_TIM_GET_COUNTER(&htim3);
	int cur_right = (int)__HAL_TIM_GET_COUNTER(&htim4);

	int d_left = cur_left - prev_left;
	int d_right = cur_right - prev_right;

	prev_left = cur_left;
	prev_right = cur_right;

	float dL = (float)d_left * MM_PER_TICK;
	float dR = (float)d_right * MM_PER_TICK;

	float d = (dL + dR) * 0.5f;
	float dTheta = (dR - dL) / WHEEL_BASE_MM;

	// Use average heading for accurate position calculation
	// This prevents accumulated heading drift from affecting position
	float avg_heading = g_odom.heading + dTheta * 0.5f;
	
	g_odom.x += d * cosf(avg_heading);
	g_odom.y += d * sinf(avg_heading);

	g_odom.heading += dTheta;

	if (g_odom.heading > (float)M_PI) g_odom.heading -= 2.0f * (float)M_PI;
	if (g_odom.heading < -(float)M_PI) g_odom.heading += 2.0f * (float)M_PI;
}

void Encoder_Reset(void) {
	g_odom.x = 0.0f;
	g_odom.y = 0.0f;
	g_odom.heading = 0.0f;

	__HAL_TIM_SET_COUNTER(&htim3, 32768);
	__HAL_TIM_SET_COUNTER(&htim4, 32768);
	prev_left = 32768;
	prev_right = 32768;
}
