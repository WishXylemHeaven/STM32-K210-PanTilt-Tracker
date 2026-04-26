#include "Types/bsp_system.h"

/*
 * 错误显示模块优化版
 *
 * ErrorQueueHandle 当前传入的是 uint8_t 错误码。
 * 本模块不改变队列结构，只负责把错误码翻译成可读信息并显示到 OLED。
 *
 * 当前错误码约定：
 * 0x01: 串口长时间没有收到数据，可能是视觉模块/上位机未发送
 * 0x02: 串口数据包格式错误，可能是协议、波特率或干扰问题
 * 0x03: 队列或内存异常，可能是 TargetQueue 满、堆内存不足等
 */

#define OLED_LINE_MAX_CHARS       16U

#define ERROR_NONE                 0x00U
#define ERROR_RX_TIMEOUT           0x01U
#define ERROR_PACKET_FORMAT        0x02U
#define ERROR_QUEUE_OR_MEMORY      0x03U

typedef struct
{
    uint8_t code;
    const char *name;
    const char *suggestion;
} ErrorDisplayInfo_t;

static const ErrorDisplayInfo_t g_error_table[] =
{
    {ERROR_RX_TIMEOUT,      "UART TIMEOUT",   "CHECK VISION RX"},
    {ERROR_PACKET_FORMAT,   "BAD PACKET",     "CHECK PROTOCOL"},
    {ERROR_QUEUE_OR_MEMORY, "QUEUE/MEM ERR",  "CHECK RAM QUEUE"},
};

static const ErrorDisplayInfo_t g_unknown_error =
{
    0xFFU,
    "UNKNOWN ERROR",
    "CHECK UART LOG"
};

static const ErrorDisplayInfo_t *FindErrorInfo(uint8_t code)
{
    uint8_t i;

    for (i = 0; i < (uint8_t)(sizeof(g_error_table) / sizeof(g_error_table[0])); i++)
    {
        if (g_error_table[i].code == code)
        {
            return &g_error_table[i];
        }
    }

    return &g_unknown_error;
}

static void OLED_ShowPaddedString(uint8_t line, const char *str)
{
    char buffer[OLED_LINE_MAX_CHARS + 1U];
    uint8_t i = 0;

    for (i = 0; i < OLED_LINE_MAX_CHARS; i++)
    {
        if ((str != NULL) && (str[i] != '\0'))
        {
            buffer[i] = str[i];
        }
        else
        {
            buffer[i] = ' ';
        }
    }

    buffer[OLED_LINE_MAX_CHARS] = '\0';
    OLED_ShowString(line, 1, buffer);
}

static void OLED_ShowNormalScreen(void)
{
    OLED_ShowPaddedString(1, "Gimbal PID OK");
    OLED_ShowPaddedString(2, "Wait UART data");
    OLED_ShowPaddedString(3, "No error yet");
    OLED_ShowPaddedString(4, "USART2 115200");
}

static void OLED_ShowErrorScreen(uint8_t error_code, uint32_t repeat_count)
{
    const ErrorDisplayInfo_t *info = FindErrorInfo(error_code);

    OLED_ShowPaddedString(1, "Gimbal PID ERR");

    OLED_ClearLine(2);
    OLED_ShowString(2, 1, "Code:0x");
    OLED_ShowHexNum(2, 8, error_code, 2);
    OLED_ShowString(2, 11, "N:");
    OLED_ShowNum(2, 13, repeat_count % 10000U, 4);

    OLED_ShowPaddedString(3, info->name);
    OLED_ShowPaddedString(4, info->suggestion);
}

/**
 * @brief 错误显示任务
 *
 * OLED 为 128x64 常见 0.96 寸屏，本工程字体为 8x16，等效 4 行 × 16 字符。
 * 优化点：
 * 1. 不再只显示 error:0xXX，而是显示错误名称和处理建议。
 * 2. 增加同类错误重复计数，方便判断错误是否持续发生。
 * 3. 使用 OLED_ClearLine / 固定 16 字符补空格，避免旧字符残留。
 * 4. 不改变 ErrorQueueHandle 队列结构，兼容原工程。
 */
void StartShowTask(void *argument)
{
    (void)argument;

    uint8_t error_code = ERROR_NONE;
    uint8_t last_error_code = ERROR_NONE;
    uint32_t repeat_count = 0;
    uint8_t has_error = 0;

    OLED_Init();
    OLED_ShowNormalScreen();

    for (;;)
    {
        osStatus_t status = osMessageQueueGet(
            ErrorQueueHandle,
            &error_code,
            NULL,
            osWaitForever
        );

        if (status == osOK)
        {
            if ((has_error != 0U) && (error_code == last_error_code))
            {
                if (repeat_count < 99999999U)
                {
                    repeat_count++;
                }
            }
            else
            {
                last_error_code = error_code;
                repeat_count = 1;
                has_error = 1;
            }

            if (error_code == ERROR_NONE)
            {
                OLED_ShowNormalScreen();
                has_error = 0;
                repeat_count = 0;
            }
            else
            {
                OLED_ShowErrorScreen(error_code, repeat_count);
            }

            printf("[ShowTask] error_code=0x%02X repeat=%lu\r\n",
                   error_code,
                   (unsigned long)repeat_count);
        }
        else
        {
            /* osWaitForever 正常情况下不会超时，这里只兜底显示未知异常。 */
            OLED_ShowErrorScreen(0xFFU, 1U);
            osDelay(100);
        }
    }
}
