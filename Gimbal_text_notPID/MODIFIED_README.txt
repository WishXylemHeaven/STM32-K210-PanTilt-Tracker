Gimbal_text_notPID 修改说明

本包已合入：
1. 串口解析任务修复：Core/App/Tesk/comman.c
   - osMessageQueueGet() 每轮只调用一次，按返回状态判断。
   - 使用状态机解析 0xaa 0x07 xH xL yH yL 0x88。

2. 目标跟踪死区：Core/App/Tesk/Tracking.c
   - DEAD_ZONE_PAN_PX = 5
   - DEAD_ZONE_TILT_PX = 5

3. 非 PID 控制保持不变
   - 本包没有加入 PID.c / PID.h。
   - Tracking.c 仍采用像素偏差换算角度后直接修正舵机角度。

4. 错误显示优化：Core/App/Tesk/Show.c，Core/App/interface/Oled.c/.h
   - OLED 显示错误码、错误含义、处理建议、重复次数。
   - 补充 OLED_ClearLine()，避免残留字符。

注意：build/Debug 内原有 bin/hex/elf 可能不是本次修改后重新编译的产物。烧录前请在 STM32CubeIDE 或 CLion + arm-none-eabi 环境中重新编译。
