#include "motor_driver.h"

void Motor_Init(void) {
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

void motor_control(int pwmL, int pwmR) {

    if (pwmR >= 0) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
    }

    if (pwmL >= 0) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
    }


    int abs_pwmL = abs(pwmL);
    int abs_pwmR = abs(pwmR);

    if (abs_pwmL > 999) abs_pwmL = 999;
    if (abs_pwmR > 999) abs_pwmR = 999;

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, abs_pwmL);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, abs_pwmR);
}
