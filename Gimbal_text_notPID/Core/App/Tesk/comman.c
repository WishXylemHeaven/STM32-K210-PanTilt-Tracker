#include "Types/bsp_system.h"

/**
 * @brief 串口解析任务
 *
 * @param argument
 */
void StartCommandTask(void *argument) {
    UART2_Receive_Start();
    uint8_t receive;
    uint8_t buffer[7];
    uint8_t index = 0;
    uint8_t sync_state = 0;
    uint8_t error_code;
    for(;;) {
        // 从队列接收一个字节
        if (osMessageQueueGet(CommandQueueHandle, &receive, 0, 10000) == osOK) {
            if (sync_state == 0) {
                // 查找包头第一个字节 0xaa
                if (receive == 0xaa) {
                    sync_state = 1;
                    buffer[0] = receive;
                }
            }
            else if (sync_state == 1) {
                // 验证包头第二个字节 0x07
                if (receive == 0x07) {
                    sync_state = 2;
                    buffer[1] = receive;
                    index = 2;
                } else {
                    // 包头不匹配，重新开始查找,并向屏幕打印错误代码
                    error_code = 0x02;
                    osMessageQueuePut(ErrorQueueHandle,&error_code,0,0);
                    sync_state = 0;
                }
            }
            else if (sync_state == 2) {
                // 接收数据部分
                buffer[index++] = receive;

                // 检查是否接收到完整的数据包
                if (index >= 7) {
                    // 验证包尾 0x88
                    if (buffer[6] == 0x88) {
                        uint16_t x_coord = ((uint16_t)buffer[2] << 8) | buffer[3];
                        uint16_t y_coord = ((uint16_t)buffer[4] << 8) | buffer[5];
                        TargetMessage* message = pvPortMalloc(sizeof(TargetMessage));
                        if (message != NULL) {
                            message->target_x = x_coord;
                            message->target_y = y_coord;
                            // 如果队列满，释放内存避免泄漏
                            if (osMessageQueuePut(TargetQueueHandle, &message, 0, 0) != osOK) {
                                vPortFree(message);
                            }
                        }
                    }
                    // 重置状态，准备接收下一个数据包
                    sync_state = 0;
                    index = 0;
                }
            }
        }
        else if(osMessageQueueGet(CommandQueueHandle, &receive, 0, 10000) == osErrorTimeout) {
            // 超时，向屏幕打印错误代码
            error_code = 0x01;
			osMessageQueuePut(ErrorQueueHandle,&error_code,0,0);
        }
        else if (osMessageQueueGet(CommandQueueHandle, &receive, 0, 10000) == osErrorNoMemory) {
            // 内存不足，向屏幕打印错误代码
			error_code = 0x03;
			osMessageQueuePut(ErrorQueueHandle,&error_code,0,0);
        }
        //uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        //printf("KeyTask stack high water mark: %u words (%u bytes)\r\n",
        //     uxHighWaterMark, uxHighWaterMark * sizeof(StackType_t));
        osDelay(10);
    }
}