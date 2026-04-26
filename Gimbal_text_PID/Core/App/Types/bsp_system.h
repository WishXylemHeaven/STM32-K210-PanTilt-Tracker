#ifndef GIMBAL_V1_BSP_SYSTEM_H
#define GIMBAL_V1_BSP_SYSTEM_H

//系统头文件说明
#include "main.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include <stdio.h>
#include <string.h>
#include "math.h"

//外设头文件声明
#include "usart.h"
#include "tim.h"
#include "interface/Oled.h"
#include "interface/Servo.h"
#include "Tesk/PID.h"
#include "oled_font.h"

//坐标队列句柄
extern osMessageQueueId_t TargetQueueHandle;
//串口缓存队列句柄
extern osMessageQueueId_t CommandQueueHandle;
//错误显示队列句柄
extern osMessageQueueId_t ErrorQueueHandle;

//坐标结构体
typedef struct {
    uint16_t target_x;
    uint16_t target_y;
}TargetMessage;

//测试堆栈大小变量
extern UBaseType_t uxHighWaterMark;

#endif //GIMBAL_V1_BSP_SYSTEM_H