import sensor, image, time, gc
from maix import KPU
from machine import UART
from fpioa_manager import fm

# ----------------------------- 可调参数 -----------------------------
SCREEN_WIDTH  = 320
SCREEN_HEIGHT = 240

# 串口发送间隔：50ms = 20Hz
SEND_INTERVAL_MS = 50

# YOLO 检测阈值
# 越低越灵敏，但误检会增加。
# 建议范围：0.18 ~ 0.25
DETECT_THRESHOLD = 0.18

# NMS 阈值
# 一般保持 0.3 即可
NMS_VALUE = 0.30

# 从无目标状态恢复到跟踪状态，需要连续检测到几帧
# 原来是 3，比较慢；这里改成 2
FACE_CONFIRM_FRAMES = 2

# 短时间丢失人脸时，继续使用上一帧坐标的时间
# 这样可以避免检测一闪一闪导致云台抖动或突然停止
LOST_HOLD_MS = 300

# 长时间无人脸后，发送无目标指令
STOP_DURATION_MS = 3000

# 坐标平滑系数
# 越大越跟手，越小越平稳
# 建议范围：0.45 ~ 0.75
SMOOTH_ALPHA = 0.60

# 每多少帧执行一次垃圾回收
GC_INTERVAL_FRAMES = 20

# ----------------------------- 硬件初始化 -----------------------------
# 串口：K210 TX -> STM32 RX
fm.register(6, fm.fpioa.UART1_RX, force=True)   # IO6 -> RXD，可不用
fm.register(8, fm.fpioa.UART1_TX, force=True)   # IO8 -> TXD
uart = UART(UART.UART1, 115200, read_buf_len=4096)

# 摄像头
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)                # 320x240

# 自动曝光/增益/白平衡：提高不同光照下的可用性
# 如果现场光线固定，也可以后续尝试改成固定曝光来稳定画面
try:
    sensor.set_auto_gain(True)
    sensor.set_auto_whitebal(True)
    sensor.set_auto_exposure(True)
except Exception as e:
    print("camera auto config warning:", e)

# 给摄像头一点时间稳定曝光和白平衡
sensor.skip_frames(time=2000)

# 时钟对象
clock = time.clock()

# 预分配 AI 输入图像
# 你的模型 net_h=256，所以这里仍然保留 320x256
od_img = image.Image(size=(320, 256))

# YOLO anchor 参数，必须和模型匹配
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
    kpu.init_yolo2(anchor,
                   anchor_num=9,
                   img_w=320,
                   img_h=240,
                   net_w=320,
                   net_h=256,
                   layer_w=10,
                   layer_h=8,
                   threshold=DETECT_THRESHOLD,
                   nms_value=NMS_VALUE,
                   classes=1)
    print("YOLO2初始化成功")
except Exception as e:
    print("YOLO2初始化失败:", e)
    while True:
        time.sleep_ms(100)

print("开始人脸检测...")

# ----------------------------- 状态机常量 -----------------------------
STATE_FACE_DETECTED = 1
STATE_SILENT        = 2
STATE_NO_TARGET     = 3

current_state = STATE_NO_TARGET

face_confirm_count = 0
lost_start_time = 0
last_send_time = 0
frame_count = 0

# 上一次有效人脸坐标
last_face_x = SCREEN_WIDTH // 2
last_face_y = SCREEN_HEIGHT // 2
has_last_face = False

# 平滑后的坐标
smooth_face_x = SCREEN_WIDTH // 2
smooth_face_y = SCREEN_HEIGHT // 2

# 预分配发送缓冲区
packet_buf = bytearray(7)

# ----------------------------- 辅助函数 -----------------------------
def face_area(det):
    """返回检测框面积，用于选择最大人脸"""
    return det[2] * det[3]


def clamp_u16(value):
    if value < 0:
        return 0xFFFF
    if value > 0xFFFF:
        return 0xFFFF
    return int(value)


def fill_packet(buf, x, y):
    """数据包：0xAA 0x07 x_h x_l y_h y_l 0x88"""
    x = clamp_u16(x)
    y = clamp_u16(y)

    buf[0] = 0xAA
    buf[1] = 0x07
    buf[2] = (x >> 8) & 0xFF
    buf[3] = x & 0xFF
    buf[4] = (y >> 8) & 0xFF
    buf[5] = y & 0xFF
    buf[6] = 0x88


def smooth_coord(old_value, new_value, alpha):
    return int(old_value * (1.0 - alpha) + new_value * alpha)


def update_smoothed_face(x, y):
    global smooth_face_x, smooth_face_y
    global last_face_x, last_face_y, has_last_face

    if not has_last_face:
        smooth_face_x = x
        smooth_face_y = y
        has_last_face = True
    else:
        smooth_face_x = smooth_coord(smooth_face_x, x, SMOOTH_ALPHA)
        smooth_face_y = smooth_coord(smooth_face_y, y, SMOOTH_ALPHA)

    last_face_x = smooth_face_x
    last_face_y = smooth_face_y


# ----------------------------- 主循环 -----------------------------
while True:
    clock.tick()
    current_time = time.ticks_ms()
    frame_count += 1

    # 不要每帧都 gc.collect，否则会带来周期性卡顿
    if frame_count % GC_INTERVAL_FRAMES == 0:
        gc.collect()

    # 获取摄像头图像
    img = sensor.snapshot()

    # 准备 AI 输入图像
    od_img.draw_image(img, 0, 0)
    od_img.pix_to_ai()

    # KPU 推理
    try:
        kpu.run_with_output(od_img)
        detections = kpu.regionlayer_yolo2()
    except Exception as e:
        print("KPU run error:", e)
        detections = None

    # 解析检测结果
    face_detected = False
    face_x = -1
    face_y = -1

    if detections:
        max_face = max(detections, key=face_area)

        try:
            x = int(max_face[0])
            y = int(max_face[1])
            w = int(max_face[2])
            h = int(max_face[3])

            face_x = x + w // 2
            face_y = y + h // 2

            # 防止偶发坐标越界
            if face_x < 0:
                face_x = 0
            elif face_x >= SCREEN_WIDTH:
                face_x = SCREEN_WIDTH - 1

            if face_y < 0:
                face_y = 0
            elif face_y >= SCREEN_HEIGHT:
                face_y = SCREEN_HEIGHT - 1

            face_detected = True

        except Exception as e:
            print("数据解析错误:", e)

    # ----------------------------- 状态机优化 -----------------------------
    if face_detected:
        update_smoothed_face(face_x, face_y)

        face_confirm_count += 1

        if face_confirm_count >= FACE_CONFIRM_FRAMES:
            current_state = STATE_FACE_DETECTED

    else:
        face_confirm_count = 0

        if current_state == STATE_FACE_DETECTED:
            current_state = STATE_SILENT
            lost_start_time = current_time

        elif current_state == STATE_SILENT:
            if time.ticks_diff(current_time, lost_start_time) >= STOP_DURATION_MS:
                current_state = STATE_NO_TARGET
                has_last_face = False

        elif current_state == STATE_NO_TARGET:
            pass

    # ----------------------------- 定时发送 -----------------------------
    if time.ticks_diff(current_time, last_send_time) >= SEND_INTERVAL_MS:

        if current_state == STATE_FACE_DETECTED:
            if has_last_face:
                # 有目标：发送平滑后的人脸坐标
                fill_packet(packet_buf, last_face_x, last_face_y)
            else:
                # 理论上很少进这里
                fill_packet(packet_buf, SCREEN_WIDTH // 2, SCREEN_HEIGHT // 2)

        elif current_state == STATE_SILENT:
            lost_time = time.ticks_diff(current_time, lost_start_time)

            if has_last_face and lost_time < LOST_HOLD_MS:
                # 短时丢帧：继续发送上一帧目标坐标
                # 这样会比立刻发中心点更跟手
                fill_packet(packet_buf, last_face_x, last_face_y)
            else:
                # 丢失时间稍长：发送画面中心点
                # 注意：这不是物理回中，只是让 STM32 不继续修正
                fill_packet(packet_buf, SCREEN_WIDTH // 2, SCREEN_HEIGHT // 2)

        elif current_state == STATE_NO_TARGET:
            # 无目标：发送 0xFFFF, 0xFFFF
            fill_packet(packet_buf, -1, -1)

        uart.write(packet_buf)
        last_send_time = current_time

    if detections:
        del detections