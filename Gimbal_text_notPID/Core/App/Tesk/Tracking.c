#include "Types/bsp_system.h"

// 摄像头参数（固定，由镜头决定）
#define IMAGE_WIDTH     320.0f   // 图像宽度（像素）
#define IMAGE_HEIGHT    240.0f   // 图像高度（像素）
#define HFOV_DEG        43.6f    // 水平视场角（度）65.5f
#define VFOV_DEG        22.6f   // 垂直视场角（度）51.2f

// 舵机角度限制（根据你的云台实际范围修改）
#define PAN_MIN         0.0f     // 旋转舵机最小角度（度）
#define PAN_MAX         270.0f   // 旋转舵机最大角度（度）
#define TILT_MIN        10.0f     // 俯仰舵机最小角度（度）
#define TILT_MAX        170.0f   // 俯仰舵机最大角度（度）


/**
 * @brief 根据图像偏移计算舵机角度偏移量
 * @param x 水平偏移（像素，正：目标在画面右侧）
 * @param y 垂直偏移（像素，正：目标在画面上方，负：下方）
 * @param delta_pan  输出：旋转舵机需转动的角度（度，正值表示向右转）
 * @param delta_tilt 输出：俯仰舵机需转动的角度（度，正值表示向上转）
 *
 * @note 该函数计算的是相对于当前指向的角度偏移，而非绝对角度。
 *       使用时，应将当前舵机角度加上此偏移，并限制在允许范围内。
 */
void CalculateAngleOffset(int16_t x, int16_t y, float *delta_pan, float *delta_tilt)
{
	// 将视场角转换为弧度
	float hfov_rad = HFOV_DEG * M_PI / 180.0f;
	float vfov_rad = VFOV_DEG * M_PI / 180.0f;

	// 计算半视场角的正切值
	float tan_half_hfov = tanf(hfov_rad / 2.0f);
	float tan_half_vfov = tanf(vfov_rad / 2.0f);

	// 水平方向：x 向右为正，但云台正角度向左转，因此取反
	float ratio_x = (float)x / (IMAGE_WIDTH / 2.0f);
	float rad_pan = atanf(ratio_x * tan_half_hfov);
	*delta_pan = -(rad_pan * 180.0f / M_PI);   // 负号使输出与 x 符号相反

	// 垂直方向：y 向下为正，云台正角度向下转，符号与 y 一致
	float ratio_y = (float)y / (IMAGE_HEIGHT / 2.0f);
	float rad_tilt = atanf(ratio_y * tan_half_vfov);
	*delta_tilt = rad_tilt * 180.0f / M_PI;    // 不取反，直接使用
	*delta_tilt=-*delta_tilt;
}

void StartTrackingTask(void *argument)
{
    /* USER CODE BEGIN StartPIDTask */
	uint16_t x=160;
	uint16_t y=120;

    //用于存储像素偏差值（转换前）
    int16_t error_pan;
	int16_t error_tilt;

	//用于存储旋转舵机需转动的角度（转换后）
    float delta_pan;
	float delta_tilt;

	float text_x;
	float text_y;

    // 设置初始角度
    TIM2_SetServoAngle_x(135.0f);
    TIM2_SetServoAngle_y(85.0f);

    // 启动TIM2通道1的PWM输出（控制偏航舵机）
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    // 启动TIM2通道2的PWM输出（控制俯仰舵机）
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

    osDelay(500);  // 等待舵机到达初始位置
    
    /* Infinite loop */
    for(;;)
    {
        TargetMessage* message;
        // 获取目标坐标（等待消息）
        osMessageQueueGet(TargetQueueHandle, &message, NULL, osWaitForever);
    	x = message->target_x;
    	y = message->target_y;
    	//printf("x:%d,y:%d\r\n",x,y);
        // 检查是否为停止指令（target_x和target_y都为65535）
        if (message->target_x == 65535 && message->target_y == 65535)
        {
            printf("Stop command received - No target detected\r\n");
            vPortFree(message);
            osDelay(100);  // 适当延时后继续循环
            continue;
        }
    	//计算像素误差
    	error_pan=x-160.0;
    	error_tilt=y-120.0;
    	//printf("error_pan:%d,error_tilt:%d\r\n",error_pan,error_tilt);
    	//将像素误差转为舵机角度误差
    	CalculateAngleOffset(error_pan,error_tilt,&delta_pan,&delta_tilt);
    	//printf("delta_pan:%d,delta_tilt:%d\r\n",(uint16_t)delta_pan,(uint16_t)delta_tilt);
    	//读取舵机当前角度
    	text_x=TIM2_GetServoAngle_x();
    	text_y=TIM2_GetServoAngle_y();
    	//printf("text_x:%d,text_y:%d\r\n",(uint16_t)text_x,(uint16_t)text_y);
    	//输出角度到舵机
    	TIM2_SetServoAngle_x(text_x+delta_pan);
    	TIM2_SetServoAngle_y(text_y+delta_tilt);
    	//printf("delta_pan:%d,delta_tilt:%d\r\n",(int16_t)delta_pan,(int16_t)delta_tilt);
        // 释放消息内存
        vPortFree(message);
        // 任务延时（40ms），确保总体周期约为50ms（消息队列约50ms来一条数据）
    	//uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    	//printf("TrackingTask stack high water mark: %u words (%u bytes)\r\n",
    	//     uxHighWaterMark, uxHighWaterMark * sizeof(StackType_t));
        osDelay(10);
    }
    /* USER CODE END StartPIDTask */
}