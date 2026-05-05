/*
 * encoder.c
 *
 *  Created on: May 5, 2026
 *      Author: MY-PC
 */

#include "encoder.h"

// TIM3 = left wheel, TIM5 = right wheel
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim5;

volatile Odometry_t g_odom = {0.0f, 0.0f, 0.0f};

// The values of the timer in the previous call
static int prev_left = 0;
static int prev_right = 0;

void Encoder_Init(void) {
	HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL); // Left encoder
	HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL); // Right encoder

	// Reset counter
	__HAL_TIM_SET_COUNTER(&htim3, 32768);
	__HAL_TIM_SET_COUNTER(&htim5, 32768);

	prev_left = 32768;
	prev_right = 32768;
}

void Encoder_Update(void) {
	int cur_left = (int)__HAL_TIM_GET_COUNTER(&htim3);
	int cur_right = (int)__HAL_TIM_GET_COUNTER(&htim5);

	int d_left = cur_left - prev_left;
	int d_right = cur_right - prev_right;

	// Save the value
	prev_left = cur_left;
	prev_right = cur_right;

	// Convert tick to mm
	float dL = (float)d_left * MM_PER_TICK;
	float dR = (float)d_right * MM_PER_TICK;

	// Calculate the distance and rotation angle
	float d = (dL + dR) * 0.5f;
	float dTheta = (dR - dL) / WHEEL_BASE_MM;

	g_odom.heading += dTheta;

	// Hold heading in [-pi. pi]
	if (g_odom.heading > (float)M_PI) g_odom.heading -= 2.0f * (float)M_PI;
	if (g_odom.heading < -(float)M_PI) g_odom.heading += 2.0f * (float)M_PI;

	g_odom.x += d * cosf(g_odom.heading);
	g_odom.y += d * sinf(g_odom.heading);
}

void Encoder_Reset(void) {
	g_odom.x = 0.0f;
	g_odom.y = 0.0f;
	g_odom.heading = 0.0f;

	__HAL_TIM_SET_COUNTER(&htim3, 32768);
	__HAL_TIM_SET_COUNTER(&htim5, 32768);
	prev_left = 32768;
	prev_right = 32768;
}
