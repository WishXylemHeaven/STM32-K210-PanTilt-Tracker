#include "Servo.h"

/**
 * @brief 将角度转换为PWM比较值（180度舵机，内部使用）
 * @param angle 角度值（0.0 ~ 180.0）
 * @return 比较值（500~2500）
 */
static uint32_t Servo_AngleToPulse_180(float angle)
{
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;
    // 500 + angle * (2000/180) 四舍五入
    return (uint32_t)(500.0f + angle * 11.111111f + 0.5f);
}

/**
 * @brief 将角度转换为PWM比较值（270度舵机，内部使用）
 * @param angle 角度值（0.0 ~ 270.0）
 * @return 比较值（500~2500）
 */
static uint32_t Servo_AngleToPulse_270(float angle)
{
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 270.0f) angle = 270.0f;
    // 500 + angle * (2000/270) = 500 + angle * 7.407407 四舍五入
    return (uint32_t)(500.0f + angle * 7.407407f + 0.5f);
}

/**
 * @brief 将PWM比较值转换为角度（180度舵机，内部使用）
 * @param pulse 比较值（500~2500）
 * @return 角度值（0.0 ~ 180.0）
 */
static float Servo_PulseToAngle_180(uint32_t pulse)
{
    if (pulse < 500) pulse = 500;
    if (pulse > 2500) pulse = 2500;
    // angle = (pulse - 500) / 11.111111f
    return (float)((pulse - 500) / 11.111111f);
}

/**
 * @brief 将PWM比较值转换为角度（270度舵机，内部使用）
 * @param pulse 比较值（500~2500）
 * @return 角度值（0.0 ~ 270.0）
 */
static float Servo_PulseToAngle_270(uint32_t pulse)
{
    if (pulse < 500) pulse = 500;
    if (pulse > 2500) pulse = 2500;
    // angle = (pulse - 500) / 7.407407f
    return (float)((pulse - 500) / 7.407407f);
}

/**
 * @brief 设置TIM2通道1的舵机角度（270度舵机）
 * @param angle 目标角度（0.0 ~ 270.0）
 */
void TIM2_SetServoAngle_x(float angle)
{
    uint32_t pulse = Servo_AngleToPulse_270(angle);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse);
}

/**
 * @brief 设置TIM2通道2的舵机角度（180度舵机）
 * @param angle 目标角度（0.0 ~ 180.0）
 */
void TIM2_SetServoAngle_y(float angle)
{
    uint32_t pulse = Servo_AngleToPulse_180(angle);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, pulse);
}

/**
 * @brief 读取TIM2通道1的当前舵机角度（270度舵机）
 * @return 当前角度值（0.0 ~ 270.0）
 */
float TIM2_GetServoAngle_x(void)
{
    uint32_t pulse = __HAL_TIM_GET_COMPARE(&htim2, TIM_CHANNEL_1);
    return Servo_PulseToAngle_270(pulse);
}

/**
 * @brief 读取TIM2通道2的当前舵机角度（180度舵机）
 * @return 当前角度值（0.0 ~ 180.0）
 */
float TIM2_GetServoAngle_y(void)
{
    uint32_t pulse = __HAL_TIM_GET_COMPARE(&htim2, TIM_CHANNEL_2);
    return Servo_PulseToAngle_180(pulse);
}