#include "Types/bsp_system.h"

// 摄像头参数
#define IMAGE_WIDTH         320.0f
#define IMAGE_HEIGHT        240.0f
#define IMAGE_CENTER_X      160
#define IMAGE_CENTER_Y      120

#define HFOV_DEG            43.6f
#define VFOV_DEG            22.6f

// 舵机角度限制
#define PAN_MIN             0.0f
#define PAN_MAX             270.0f
#define TILT_MIN            10.0f
#define TILT_MAX            170.0f

// 死区：目标在中心附近多少像素内不修正
#define DEAD_ZONE_PAN_PX    5
#define DEAD_ZONE_TILT_PX   5

// 初始角度
#define PAN_INIT_ANGLE      135.0f
#define TILT_INIT_ANGLE     85.0f

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
 * @brief 根据图像偏移计算舵机角度偏移量
 *
 * @param x 水平偏移，单位：像素。正数表示目标在画面右侧。
 * @param y 垂直偏移，单位：像素。正数表示目标在画面下方。
 * @param delta_pan 输出：水平舵机角度修正量。
 * @param delta_tilt 输出：俯仰舵机角度修正量。
 */
void CalculateAngleOffset(int16_t x, int16_t y, float *delta_pan, float *delta_tilt)
{
    float hfov_rad = HFOV_DEG * M_PI / 180.0f;
    float vfov_rad = VFOV_DEG * M_PI / 180.0f;

    float tan_half_hfov = tanf(hfov_rad / 2.0f);
    float tan_half_vfov = tanf(vfov_rad / 2.0f);

    /*
     * 水平方向：
     * x > 0 表示目标在画面右侧。
     * 原工程这里取了负号，保持原来的舵机方向不变。
     */
    float ratio_x = (float)x / (IMAGE_WIDTH / 2.0f);
    float rad_pan = atanf(ratio_x * tan_half_hfov);
    *delta_pan = -(rad_pan * 180.0f / M_PI);

    /*
     * 垂直方向：
     * y > 0 表示目标在画面下方。
     * 原工程最后对 delta_tilt 取反，这里保留原方向。
     */
    float ratio_y = (float)y / (IMAGE_HEIGHT / 2.0f);
    float rad_tilt = atanf(ratio_y * tan_half_vfov);
    *delta_tilt = -(rad_tilt * 180.0f / M_PI);
}

void StartTrackingTask(void *argument)
{
    (void)argument;

    uint16_t x = IMAGE_CENTER_X;
    uint16_t y = IMAGE_CENTER_Y;

    int16_t error_pan = 0;
    int16_t error_tilt = 0;

    float delta_pan = 0.0f;
    float delta_tilt = 0.0f;

    float current_pan = 0.0f;
    float current_tilt = 0.0f;

    float target_pan = 0.0f;
    float target_tilt = 0.0f;

    /*
     * 先设置初始比较值，再启动 PWM。
     */
    TIM2_SetServoAngle_x(PAN_INIT_ANGLE);
    TIM2_SetServoAngle_y(TILT_INIT_ANGLE);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

    osDelay(500);

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

        /*
         * 停止指令：
         * x = 65535
         * y = 65535
         */
        if ((x == 65535U) && (y == 65535U))
        {
            printf("Stop command received - No target detected\r\n");

            vPortFree(message);
            osDelay(100);
            continue;
        }

        /*
         * 计算像素误差。
         * x > 160：目标在画面右侧。
         * y > 120：目标在画面下方。
         */
        error_pan = (int16_t)x - IMAGE_CENTER_X;
        error_tilt = (int16_t)y - IMAGE_CENTER_Y;

        /*
         * 加入死区：
         * 目标在中心附近小范围抖动时，不让舵机跟着抖。
         */
        error_pan = ApplyDeadZone(error_pan, DEAD_ZONE_PAN_PX);
        error_tilt = ApplyDeadZone(error_tilt, DEAD_ZONE_TILT_PX);

        /*
         * 如果两个方向都在死区内，本轮不更新舵机。
         */
        if ((error_pan == 0) && (error_tilt == 0))
        {
            vPortFree(message);
            osDelay(10);
            continue;
        }

        /*
         * 将像素误差转换为角度误差。
         */
        CalculateAngleOffset(error_pan, error_tilt, &delta_pan, &delta_tilt);

        /*
         * 读取当前角度。
         * 注意：这里读取的是当前 PWM 比较值反推的角度，
         * 不是舵机真实反馈角度。
         */
        current_pan = TIM2_GetServoAngle_x();
        current_tilt = TIM2_GetServoAngle_y();

        /*
         * 计算目标角度，并加入软限位。
         */
        target_pan = ClampFloat(current_pan + delta_pan, PAN_MIN, PAN_MAX);
        target_tilt = ClampFloat(current_tilt + delta_tilt, TILT_MIN, TILT_MAX);

        /*
         * 输出到舵机。
         */
        TIM2_SetServoAngle_x(target_pan);
        TIM2_SetServoAngle_y(target_tilt);

        vPortFree(message);

        osDelay(10);
    }
}
