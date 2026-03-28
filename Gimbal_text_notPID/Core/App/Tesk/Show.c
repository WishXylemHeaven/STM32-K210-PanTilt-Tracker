#include "Types/bsp_system.h"

/*
 *错误类型表示
 *0x01代表串口超过10秒没收到信息，可能是模块未上线
 *0x02代表串口接收到乱码
 *0x03代表系统内存不足
 */
/**
 * @brief 错误显示任务
 *
 * @param argument
 */
void StartShowTask(void *argument) {
	OLED_Init();
	uint8_t error_code;
	OLED_ShowString(1, 1, "hello world");
	for (;;) {
		osMessageQueueGet(ErrorQueueHandle,&error_code,0,osWaitForever);
		OLED_ShowString(2,1,"error:");
		OLED_ShowHexNum(2,7,error_code,2);
		/*
		TargetMessage* message;
		osMessageQueueGet(TargetQueueHandle,&message,0,osWaitForever);
		uint16_t x = message->target_x;
		uint16_t y = message->target_y;
		OLED_ShowString(2, 1, "X:");
		OLED_ShowNum(2, 3, x, 5);
		OLED_ShowString(3, 1, "Y:");
		OLED_ShowNum(3, 3, y, 5);
		OLED_ShowChar(4,1,'A');
		vPortFree(message);
		*/
		//uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
		//printf("ShowTask stack high water mark: %u words (%u bytes)\r\n",
		//     uxHighWaterMark, uxHighWaterMark * sizeof(StackType_t));
		 osDelay(10);
	}

}