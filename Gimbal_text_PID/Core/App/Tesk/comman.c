#include "Types/bsp_system.h"

/*
 * 串口协议：
 * 0xaa 0x07 x_h x_l y_h y_l 0x88
 *
 * 特殊停止指令：
 * x = 65535
 * y = 65535
 */

#define CMD_PACKET_HEAD1        0xAAU
#define CMD_PACKET_HEAD2        0x07U
#define CMD_PACKET_TAIL         0x88U
#define CMD_PACKET_LEN          7U

#define CMD_RX_TIMEOUT_TICKS    10000U

#define ERROR_RX_TIMEOUT        0x01U
#define ERROR_PACKET_FORMAT     0x02U
#define ERROR_QUEUE_OR_MEMORY   0x03U

typedef enum
{
    RX_WAIT_HEAD1 = 0,
    RX_WAIT_HEAD2,
    RX_READ_BODY
} CommandRxState_t;

static void SendErrorCode(uint8_t error_code)
{
    if (ErrorQueueHandle != NULL)
    {
        (void)osMessageQueuePut(ErrorQueueHandle, &error_code, 0, 0);
    }
}

static void SendTargetMessage(uint16_t x_coord, uint16_t y_coord)
{
    TargetMessage *message = (TargetMessage *)pvPortMalloc(sizeof(TargetMessage));

    if (message == NULL)
    {
        SendErrorCode(ERROR_QUEUE_OR_MEMORY);
        return;
    }

    message->target_x = x_coord;
    message->target_y = y_coord;

    /*
     * TargetQueue 创建时保存的是 TargetMessage*，
     * 所以这里放入的是 message 指针的地址。
     */
    if (osMessageQueuePut(TargetQueueHandle, &message, 0, 0) != osOK)
    {
        vPortFree(message);
        SendErrorCode(ERROR_QUEUE_OR_MEMORY);
    }
}

/**
 * @brief 串口解析任务
 *
 * 从 CommandQueueHandle 逐字节取出 USART2 接收到的数据，
 * 按 7 字节协议解析成 TargetMessage，然后发送到 TargetQueueHandle。
 */
void StartCommandTask(void *argument)
{
    (void)argument;

    UART2_Receive_Start();

    uint8_t receive = 0;
    uint8_t buffer[CMD_PACKET_LEN] = {0};
    uint8_t index = 0;
    CommandRxState_t state = RX_WAIT_HEAD1;

    for (;;)
    {
        /*
         * 关键修复点：
         * 这里只调用一次 osMessageQueueGet，
         * 然后根据 status 判断结果，避免一次循环误取多次队列数据。
         */
        osStatus_t status = osMessageQueueGet(
            CommandQueueHandle,
            &receive,
            NULL,
            CMD_RX_TIMEOUT_TICKS
        );

        if (status == osOK)
        {
            switch (state)
            {
                case RX_WAIT_HEAD1:
                {
                    if (receive == CMD_PACKET_HEAD1)
                    {
                        buffer[0] = receive;
                        state = RX_WAIT_HEAD2;
                    }
                    break;
                }

                case RX_WAIT_HEAD2:
                {
                    if (receive == CMD_PACKET_HEAD2)
                    {
                        buffer[1] = receive;
                        index = 2;
                        state = RX_READ_BODY;
                    }
                    else if (receive == CMD_PACKET_HEAD1)
                    {
                        /* 连续收到 0xaa 0xaa 时，后一个仍可作为新包头。 */
                        buffer[0] = receive;
                        state = RX_WAIT_HEAD2;
                    }
                    else
                    {
                        SendErrorCode(ERROR_PACKET_FORMAT);
                        state = RX_WAIT_HEAD1;
                        index = 0;
                    }
                    break;
                }

                case RX_READ_BODY:
                {
                    buffer[index++] = receive;

                    if (index >= CMD_PACKET_LEN)
                    {
                        if (buffer[6] == CMD_PACKET_TAIL)
                        {
                            uint16_t x_coord = ((uint16_t)buffer[2] << 8) | buffer[3];
                            uint16_t y_coord = ((uint16_t)buffer[4] << 8) | buffer[5];

                            SendTargetMessage(x_coord, y_coord);
                        }
                        else
                        {
                            SendErrorCode(ERROR_PACKET_FORMAT);
                        }

                        state = RX_WAIT_HEAD1;
                        index = 0;
                    }
                    break;
                }

                default:
                {
                    state = RX_WAIT_HEAD1;
                    index = 0;
                    break;
                }
            }
        }
        else if (status == osErrorTimeout)
        {
            /* 超时说明较长时间没有收到串口字节，同时清掉半包状态。 */
            SendErrorCode(ERROR_RX_TIMEOUT);
            state = RX_WAIT_HEAD1;
            index = 0;
        }
        else
        {
            SendErrorCode(ERROR_QUEUE_OR_MEMORY);
            state = RX_WAIT_HEAD1;
            index = 0;
            osDelay(10);
        }
    }
}
