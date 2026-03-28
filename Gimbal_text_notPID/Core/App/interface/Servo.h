

#ifndef GIMBAL_V1_SERVO_H
#define GIMBAL_V1_SERVO_H

#include "Types/bsp_system.h"

extern TIM_HandleTypeDef htim2;

/**
 * @brief 设置TIM2通道1的舵机角度 x轴舵机角度（270度舵机）
 * @param angle 目标角度（0.0 ~ 270.0）
 */
void TIM2_SetServoAngle_x(float angle);

/**
 * @brief 设置TIM2通道2的舵机角度 y轴舵机角度（180度舵机）
 * @param angle 目标角度（0.0 ~ 180.0）
 */
void TIM2_SetServoAngle_y(float angle);

/**
 * @brief 读取TIM2通道1的当前舵机角度 x轴舵机角度（270度舵机）
 * @return 当前角度值（0.0 ~ 270.0）
 */
float TIM2_GetServoAngle_x(void);

/**
 * @brief 读取TIM2通道2的当前舵机角度 y轴舵机角度（180度舵机）
 * @return 当前角度值（0.0 ~ 180.0）
 */
float TIM2_GetServoAngle_y(void);

#endif //GIMBAL_V1_SERVO_H