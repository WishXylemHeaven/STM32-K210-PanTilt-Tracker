Gimbal_text_PID 修改说明

本压缩包基于原 Gimbal_text_notPID 工程生成，保留原工程目录结构，并增加 PID 版本的云台跟踪控制。

主要改动：
1. 新增 Core/App/Tesk/PID.h
2. 新增 Core/App/Tesk/PID.c
3. 修改 Core/App/Tesk/Tracking.c：
   - 加入像素死区
   - 将像素误差换算为角度误差
   - 使用 PID 输出每个控制周期的角度修正量
   - 加入输出限幅和积分限幅
   - 无目标/进入死区时重置 PID 历史
4. 修改 Core/App/Tesk/comman.c：
   - 修复 osMessageQueueGet 被重复调用的问题
   - 改为状态机解析 0xaa 0x07 ... 0x88 数据包
5. 修改 CMakeLists.txt：
   - 将 Core/App/Tesk/PID.c 加入编译列表
   - 额外加入 Core/App 作为包含路径，避免大小写路径问题
6. 修改 Core/App/Types/bsp_system.h：
   - 加入 Tesk/PID.h

注意：
- 原 build/Debug 下的 .bin/.hex/.elf 是原工程已有构建产物，本次未在服务器上重新交叉编译。
- 请在 STM32CubeIDE、CLion + CMake、或你的本地 arm-none-eabi 工具链中重新编译后再烧录。
- PID 参数位于 Core/App/Tesk/Tracking.c 顶部，可根据实际舵机响应微调。
