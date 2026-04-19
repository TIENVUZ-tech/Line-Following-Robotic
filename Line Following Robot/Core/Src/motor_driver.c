#include "motor_driver.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Khoi tao PWM4
void Motor_Init(void) {
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

/// Dieu khien chieu quay va toc do dong co
void motor_control(int pwmL, int pwmR) {
    // 1. Xu ly chieu quay banh trai - IN1(PC7) & IN2(PA9)
    if (pwmL >= 0) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);   // Tien
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET); // Lui
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
    }

    // 2. Xử lý chiều quay bánh phải - IN3(PA8) & IN4(PB10)
    if (pwmR >= 0) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);   // Tien
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET); // Lui
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
    }

    // 3. Trị tuyệt đối xung PWM
    int abs_pwmL = abs(pwmL);
    int abs_pwmR = abs(pwmR);

    // 4. Giới hạn chống tràn (0 - 100)
    if (abs_pwmL > 100) abs_pwmL = 100;
    if (abs_pwmR > 100) abs_pwmR = 100;

    // 5. Xuất tín hiệu ra chân PWM (Nhân 5 để max công suất với Period 499)
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, abs_pwmL * 5); // ENA - Trai
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, abs_pwmR * 5); // ENB - Phai
}

