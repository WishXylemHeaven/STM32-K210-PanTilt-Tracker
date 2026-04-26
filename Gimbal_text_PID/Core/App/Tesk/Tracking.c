#include "Types/bsp_system.h"

// 摄像头参数（固定，由镜头决定）
#define IMAGE_WIDTH             320.0f
#define IMAGE_HEIGHT            240.0f
#define IMAGE_CENTER_X          160
#define IMAGE_CENTER_Y          120

#define HFOV_DEG                43.6f
#define VFOV_DEG                22.6f

// 舵机角度限制（根据你的云台实际范围修改）
#define PAN_MIN                 0.0f
#define PAN_MAX                 270.0f
#define TILT_MIN                10.0f
#define TILT_MAX                170.0f

// 死区：目标在中心附近多少像素内不修正
#define DEAD_ZONE_PAN_PX        5
#define DEAD_ZONE_TILT_PX       5

// 初始角度
#define PAN_INIT_ANGLE          135.0f
#define TILT_INIT_ANGLE         85.0f

// 控制周期：TrackingTask 最后 osDelay(10)，所以这里按 10ms 设计
#define PID_SAMPLE_TIME_S       0.010f

// PID 参数：先给一组偏保守、适合首轮上电测试的参数
// 单位说明：PID 输入是角度误差（度），输出是本周期角度修正量（度）
#define PID_PAN_KP              0.70f
#define PID_PAN_KI              0.02f
#define PID_PAN_KD              0.04f

#define PID_TILT_KP             0.65f
#define PID_TILT_KI             0.02f
#define PID_TILT_KD             0.035f

// 限制单次更新最大角度，避免目标跳变时舵机猛甩
#define PID_PAN_OUTPUT_LIMIT    3.0f
#define PID_TILT_OUTPUT_LIMIT   2.5f

// 积分项限幅，防止积分饱和
#define PID_PAN_INTEGRAL_LIMIT  1.5f
#define PID_TILT_INTEGRAL_LIMIT 1.2f

static float ClampFloat(float value, float min_value, float max_value)
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

static int16_t ApplyDeadZone(int16_t error, int16_t dead_zone)
{
    if ((error <= dead_zone) && (error >= -dead_zone))
    {
        return 0;
    }

    return error;
}

/**
 * @brief 根据图像偏移计算云台需要修正的角度误差
 *
 * @param x 水平偏移，单位：像素。正数表示目标在画面右侧。
 * @param y 垂直偏移，单位：像素。正数表示目标在画面下方。
 * @param angle_error_pan 输出：水平修正角度误差。
 * @param angle_error_tilt 输出：俯仰修正角度误差。
 *
 * @note 这里输出的是“角度误差”，后面再交给 PID 计算本周期实际修正量。
 */
void CalculateAngleOffset(int16_t x, int16_t y, float *angle_error_pan, float *angle_error_tilt)
{
    float hfov_rad = HFOV_DEG * M_PI / 180.0f;
    float vfov_rad = VFOV_DEG * M_PI / 180.0f;

    float tan_half_hfov = tanf(hfov_rad / 2.0f);
    float tan_half_vfov = tanf(vfov_rad / 2.0f);

    /*
     * 水平方向：
     * x > 0 表示目标在画面右侧。
     * 原工程这里取负号，说明舵机角度正方向与画面 x 正方向相反。
     * 为保持你原来的机械方向不变，这里继续保留负号。
     */
    float ratio_x = (float)x / (IMAGE_WIDTH / 2.0f);
    float rad_pan = atanf(ratio_x * tan_half_hfov);
    *angle_error_pan = -(rad_pan * 180.0f / M_PI);

    /*
     * 垂直方向：
     * 原工程最终等效为负号，这里同样保持原方向不变。
     */
    float ratio_y = (float)y / (IMAGE_HEIGHT / 2.0f);
    float rad_tilt = atanf(ratio_y * tan_half_vfov);
    *angle_error_tilt = -(rad_tilt * 180.0f / M_PI);
}

void StartTrackingTask(void *argument)
{
    (void)argument;

    uint16_t x = IMAGE_CENTER_X;
    uint16_t y = IMAGE_CENTER_Y;

    int16_t error_pan_px = 0;
    int16_t error_tilt_px = 0;

    float angle_error_pan = 0.0f;
    float angle_error_tilt = 0.0f;

    float pid_output_pan = 0.0f;
    float pid_output_tilt = 0.0f;

    float current_pan = 0.0f;
    float current_tilt = 0.0f;

    float target_pan = 0.0f;
    float target_tilt = 0.0f;

    PIDController pan_pid;
    PIDController tilt_pid;

    PID_Init(&pan_pid, PID_PAN_KP, PID_PAN_KI, PID_PAN_KD);
    PID_SetOutputLimits(&pan_pid, -PID_PAN_OUTPUT_LIMIT, PID_PAN_OUTPUT_LIMIT);
    PID_SetIntegralLimits(&pan_pid, -PID_PAN_INTEGRAL_LIMIT, PID_PAN_INTEGRAL_LIMIT);

    PID_Init(&tilt_pid, PID_TILT_KP, PID_TILT_KI, PID_TILT_KD);
    PID_SetOutputLimits(&tilt_pid, -PID_TILT_OUTPUT_LIMIT, PID_TILT_OUTPUT_LIMIT);
    PID_SetIntegralLimits(&tilt_pid, -PID_TILT_INTEGRAL_LIMIT, PID_TILT_INTEGRAL_LIMIT);

    // 设置初始角度
    TIM2_SetServoAngle_x(PAN_INIT_ANGLE);
    TIM2_SetServoAngle_y(TILT_INIT_ANGLE);

    // 启动 TIM2 通道 1、2 的 PWM 输出
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

    osDelay(500);  // 等待舵机到达初始位置

    for (;;)
    {
        TargetMessage *message = NULL;

        if (osMessageQueueGet(TargetQueueHandle, &message, NULL, osWaitForever) != osOK)
        {
            osDelay(10);
            continue;
        }

        if (message == NULL)
        {
            osDelay(10);
            continue;
        }

        x = message->target_x;
        y = message->target_y;

        // 检查是否为停止指令（target_x 和 target_y 都为 65535）
        if ((x == 65535U) && (y == 65535U))
        {
            printf("Stop command received - No target detected\r\n");

            // 无目标时清空 PID 历史，避免下次出现目标时积分项残留
            PID_Reset(&pan_pid);
            PID_Reset(&tilt_pid);

            vPortFree(message);
            osDelay(100);
            continue;
        }

        // 计算像素误差
        error_pan_px = (int16_t)x - IMAGE_CENTER_X;
        error_tilt_px = (int16_t)y - IMAGE_CENTER_Y;

        // 加入死区：中心附近的小抖动不让舵机动作
        error_pan_px = ApplyDeadZone(error_pan_px, DEAD_ZONE_PAN_PX);
        error_tilt_px = ApplyDeadZone(error_tilt_px, DEAD_ZONE_TILT_PX);

        // 某一轴进入死区时，清掉该轴 PID 历史，防止积分继续累积
        if (error_pan_px == 0)
        {
            PID_Reset(&pan_pid);
        }

        if (error_tilt_px == 0)
        {
            PID_Reset(&tilt_pid);
        }

        // 如果两个方向都在死区内，本轮不更新舵机
        if ((error_pan_px == 0) && (error_tilt_px == 0))
        {
            vPortFree(message);
            osDelay(10);
            continue;
        }

        // 将像素误差转换为角度误差
        CalculateAngleOffset(error_pan_px, error_tilt_px, &angle_error_pan, &angle_error_tilt);

        // PID 计算本周期舵机角度修正量
        if (error_pan_px != 0)
        {
            pid_output_pan = PID_Update(&pan_pid, angle_error_pan, PID_SAMPLE_TIME_S);
        }
        else
        {
            pid_output_pan = 0.0f;
        }

        if (error_tilt_px != 0)
        {
            pid_output_tilt = PID_Update(&tilt_pid, angle_error_tilt, PID_SAMPLE_TIME_S);
        }
        else
        {
            pid_output_tilt = 0.0f;
        }

        // 读取当前角度。注意：这是由 PWM 比较值反推的角度，不是真实反馈角度。
        current_pan = TIM2_GetServoAngle_x();
        current_tilt = TIM2_GetServoAngle_y();

        // 计算目标角度，并加入软限位
        target_pan = ClampFloat(current_pan + pid_output_pan, PAN_MIN, PAN_MAX);
        target_tilt = ClampFloat(current_tilt + pid_output_tilt, TILT_MIN, TILT_MAX);

        // 输出角度到舵机
        TIM2_SetServoAngle_x(target_pan);
        TIM2_SetServoAngle_y(target_tilt);

        vPortFree(message);

        osDelay(10);
    }
}
