import sensor, image, time, gc
from maix import KPU
from machine import UART
from fpioa_manager import fm

# ----------------------------- 硬件初始化 -----------------------------
# 串口（仅用于发送，不再接收）
fm.register(6, fm.fpioa.UART1_RX, force=True)   # IO6 -> RXD
fm.register(8, fm.fpioa.UART1_TX, force=True)   # IO8 -> TXD
uart = UART(UART.UART1, 115200, read_buf_len=4096)

# 摄像头
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)                # 320x240
sensor.skip_frames(time=100)

# 屏幕尺寸（用于中点坐标）
SCREEN_WIDTH  = 320
SCREEN_HEIGHT = 240

# 静默状态最长持续时间（毫秒）
STOP_DURATION_MS = 5000

# 时钟对象（用于帧率统计，可选）
clock = time.clock()

# 预分配一个用于AI推理的图像对象（320x256）
od_img = image.Image(size=(320, 256))

# YOLO anchor参数（保持原样）
anchor = (0.893, 1.463, 0.245, 0.389, 1.55, 2.58, 0.375, 0.594,
          3.099, 5.038, 0.057, 0.090, 0.567, 0.904, 0.101, 0.160,
          0.159, 0.255)

# ----------------------------- KPU 初始化 -----------------------------
kpu = KPU()

try:
    kpu.load_kmodel("/sd/KPU/yolo_face_detect/yolo_face_detect.kmodel")
    print("模型加载成功")
except Exception as e:
    print("模型加载失败，请检查文件路径:", e)
    while True:
        time.sleep_ms(100)

try:
    kpu.init_yolo2(anchor, anchor_num=9, img_w=320, img_h=240,
                   net_w=320, net_h=256, layer_w=10, layer_h=8,
                   threshold=0.25, nms_value=0.3, classes=1)
    print("YOLO2初始化成功")
except Exception as e:
    print("YOLO2初始化失败:", e)
    while True:
        time.sleep_ms(100)

print("开始人脸检测...")

# ----------------------------- 状态机常量 -----------------------------
STATE_FACE_DETECTED = 1   # 有目标
STATE_SILENT        = 2   # 静默状态（无人脸超时前不发数据）
STATE_NO_TARGET     = 3   # 无目标状态

current_state      = STATE_NO_TARGET
silent_start_time  = 0
no_target_face_count = 0
last_send_time     = 0   # 上次发送数据包的时间戳

# ----------------------------- 辅助函数 -----------------------------
def face_area(det):
    """返回检测框的面积（宽度 * 高度），用于选择最大人脸"""
    return det[2] * det[3]

# 预分配发送缓冲区，避免循环内重复创建
packet_buf = bytearray(7)

def fill_packet(buf, x, y):
    """将坐标填充到7字节数据包：0xAA 0x07 x_h x_l y_h y_l 0x88"""
    buf[0] = 0xAA
    buf[1] = 0x07
    # 坐标限制在 [0, 0xFFFF] 区间，-1 转换为 0xFFFF
    if x < 0:
        x = 0xFFFF
    elif x > 0xFFFF:
        x = 0xFFFF
    buf[2] = (x >> 8) & 0xFF
    buf[3] = x & 0xFF
    if y < 0:
        y = 0xFFFF
    elif y > 0xFFFF:
        y = 0xFFFF
    buf[4] = (y >> 8) & 0xFF
    buf[5] = y & 0xFF
    buf[6] = 0x88

# ----------------------------- 主循环 -----------------------------
while True:
    gc.collect()                     # 定期回收内存
    current_time = time.ticks_ms()

    # 获取摄像头图像
    img = sensor.snapshot()

    # 准备AI输入图像
    od_img.draw_image(img, 0, 0)
    od_img.pix_to_ai()

    # 执行KPU推理
    try:
        kpu.run_with_output(od_img)
        detections = kpu.regionlayer_yolo2()
    except Exception:
        detections = []

    # 解析检测结果
    face_detected = False
    face_x = -1
    face_y = -1
    if detections:
        # 找到面积最大的检测框
        max_face = max(detections, key=face_area)
        try:
            x, y, w, h = int(max_face[0]), int(max_face[1]), int(max_face[2]), int(max_face[3])
            face_x = x + w // 2
            face_y = y + h // 2
            face_detected = True
        except Exception as e:
            print("数据解析错误:", e)

    # ----------------------------- 状态机（无自动发送） -----------------------------
    if current_state == STATE_FACE_DETECTED:
        if not face_detected:
            current_state = STATE_SILENT
            silent_start_time = current_time
    elif current_state == STATE_SILENT:
        if face_detected:
            current_state = STATE_FACE_DETECTED
        else:
            if time.ticks_diff(current_time, silent_start_time) >= STOP_DURATION_MS:
                current_state = STATE_NO_TARGET
                no_target_face_count = 0
    elif current_state == STATE_NO_TARGET:
        if face_detected:
            no_target_face_count += 1
            if no_target_face_count >= 3:
                current_state = STATE_FACE_DETECTED
                no_target_face_count = 0
        else:
            no_target_face_count = 0

    # ----------------------------- 定时发送（每50ms一次） -----------------------------
    # 检查是否到达发送间隔（1秒20次 = 50ms）
    if time.ticks_diff(current_time, last_send_time) >= 100:
        # 根据当前状态和检测结果填充数据包
        # 修改点1: STATE_FACE_DETECTED 下，若无人脸则发送中点
        # 修改点2: STATE_NO_TARGET 下，无论是否有人脸，均发送 (-1, -1)
        if current_state == STATE_SILENT:
            # 静默状态：返回画面中点坐标（云台回中）
            fill_packet(packet_buf, SCREEN_WIDTH // 2, SCREEN_HEIGHT // 2)
        elif current_state == STATE_FACE_DETECTED:
            if face_detected:
                fill_packet(packet_buf, face_x, face_y)
            else:
                # 有目标但当前帧未检测到人脸 → 发送中点，使云台回中
                fill_packet(packet_buf, SCREEN_WIDTH // 2, SCREEN_HEIGHT // 2)
        elif current_state == STATE_NO_TARGET:
            # 无目标状态：始终发送无效坐标，即使检测到人脸也不提前跟踪
            fill_packet(packet_buf, -1, -1)
        # 发送数据包
        uart.write(packet_buf)
        last_send_time = current_time

    # 显式删除临时列表，帮助垃圾回收
    if 'detections' in locals():
        del detections

# 程序不会执行到这里，仅作为清理参考
kpu.deinit()
