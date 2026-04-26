#include "Tesk/PID.h"

static float PID_Clamp(float value, float min_value, float max_value)
{
    if (value < min_value)
    {
        return min_value;
    }

    if (value > max_value)
    {
        return max_value;
    }

    return value;
}

void PID_Init(PIDController *pid, float kp, float ki, float kd)
{
    if (pid == 0)
    {
        return;
    }

    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;

    pid->integral = 0.0f;
    pid->last_error = 0.0f;

    pid->output_min = -1000000.0f;
    pid->output_max = 1000000.0f;

    pid->integral_min = -1000000.0f;
    pid->integral_max = 1000000.0f;

    pid->first_run = 1U;
}

void PID_SetTunings(PIDController *pid, float kp, float ki, float kd)
{
    if (pid == 0)
    {
        return;
    }

    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
}

void PID_SetOutputLimits(PIDController *pid, float min_output, float max_output)
{
    if (pid == 0)
    {
        return;
    }

    if (min_output > max_output)
    {
        float temp = min_output;
        min_output = max_output;
        max_output = temp;
    }

    pid->output_min = min_output;
    pid->output_max = max_output;
}

void PID_SetIntegralLimits(PIDController *pid, float min_integral, float max_integral)
{
    if (pid == 0)
    {
        return;
    }

    if (min_integral > max_integral)
    {
        float temp = min_integral;
        min_integral = max_integral;
        max_integral = temp;
    }

    pid->integral_min = min_integral;
    pid->integral_max = max_integral;
}

void PID_Reset(PIDController *pid)
{
    if (pid == 0)
    {
        return;
    }

    pid->integral = 0.0f;
    pid->last_error = 0.0f;
    pid->first_run = 1U;
}

/**
 * @brief PID 更新函数
 *
 * @param pid PID 控制器对象
 * @param error 当前误差。此工程中使用“目标角度修正误差”，单位为度。
 * @param dt_s 控制周期，单位：秒。
 * @return PID 输出。此工程中输出为本周期舵机角度修正量，单位为度。
 */
float PID_Update(PIDController *pid, float error, float dt_s)
{
    if ((pid == 0) || (dt_s <= 0.0f))
    {
        return 0.0f;
    }

    float p_out = pid->kp * error;

    /* 积分项直接保存为输出量，便于限幅和防积分饱和。 */
    pid->integral += pid->ki * error * dt_s;
    pid->integral = PID_Clamp(pid->integral, pid->integral_min, pid->integral_max);

    float derivative = 0.0f;
    if (pid->first_run != 0U)
    {
        pid->first_run = 0U;
        derivative = 0.0f;
    }
    else
    {
        derivative = (error - pid->last_error) / dt_s;
    }

    float d_out = pid->kd * derivative;

    pid->last_error = error;

    float output = p_out + pid->integral + d_out;
    output = PID_Clamp(output, pid->output_min, pid->output_max);

    return output;
}
