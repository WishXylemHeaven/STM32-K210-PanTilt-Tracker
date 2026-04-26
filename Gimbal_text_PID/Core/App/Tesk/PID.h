#ifndef GIMBAL_V1_PID_H
#define GIMBAL_V1_PID_H

#include <stdint.h>

typedef struct
{
    float kp;
    float ki;
    float kd;

    float integral;
    float last_error;

    float output_min;
    float output_max;

    float integral_min;
    float integral_max;

    uint8_t first_run;
} PIDController;

void PID_Init(PIDController *pid, float kp, float ki, float kd);
void PID_SetTunings(PIDController *pid, float kp, float ki, float kd);
void PID_SetOutputLimits(PIDController *pid, float min_output, float max_output);
void PID_SetIntegralLimits(PIDController *pid, float min_integral, float max_integral);
void PID_Reset(PIDController *pid);
float PID_Update(PIDController *pid, float error, float dt_s);

#endif /* GIMBAL_V1_PID_H */
