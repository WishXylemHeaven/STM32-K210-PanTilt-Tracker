# STM32-K210 PanTilt Tracker

## Overview

This project is a low-cost two-axis face-tracking pan-tilt system. The K210 module captures camera images, detects faces, and calculates the target center point. The STM32 controller receives the target coordinates and drives two servos to keep the pan-tilt platform aligned with the target.

The current version implements face detection, coordinate transmission, and pan-tilt tracking. Future work will add face recognition and LoRa-based remote status reporting, turning the project into a low-cost intelligent monitoring system.

## System Architecture

```text
K210 camera module
    ↓ face detection / coordinate calculation
UART serial communication
    ↓ target coordinate packet
STM32 pan-tilt controller
    ↓ PWM
Two-axis servo pan-tilt platform
```

## Current Features

- K210 runs a YOLO2 face detection model.
- The largest detected face is selected as the tracking target.
- The center point `(x, y)` of the target face is calculated.
- A 7-byte UART packet is sent to the STM32.
- STM32 parses the packet and controls the pan and tilt servos.
- Both PID and non-PID firmware versions are provided.
- Dead zone handling, UART parser fixes, and improved error display are included.
- When no target is detected, `0xFFFF, 0xFFFF` is sent to notify the STM32 to stop tracking.

## UART Protocol

```text
0xAA 0x07 XH XL YH YL 0x88
```

| Byte | Meaning |
|---|---|
| `0xAA` | Header 1 |
| `0x07` | Header 2 / length marker |
| `XH XL` | Target center X coordinate, high byte first |
| `YH YL` | Target center Y coordinate, high byte first |
| `0x88` | Tail byte |

No-target command:

```text
X = 65535
Y = 65535
```

## Algorithm

### 1. Face Detection

The K210 captures 320×240 images and uses the KPU to accelerate a YOLO2 face detection model. If multiple faces are detected, the largest face is selected as the target.

### 2. Coordinate Calculation

For a detected bounding box `(x, y, w, h)`, the target center point is calculated as:

```text
face_x = x + w / 2
face_y = y + h / 2
```

### 3. Pan-Tilt Control

The STM32 compares the target coordinate with the image center `(160, 120)`, calculates the pixel error, and converts it into servo angle correction.

The non-PID version directly applies the angle correction. The PID version uses proportional, integral, and derivative control to make tracking smoother.

### 4. Dead Zone

When the target is close to the image center, small errors are ignored to prevent the servos from shaking due to detection noise.

## Repository Structure

```text
Gimbal_text_PID/       STM32 firmware with PID control
Gimbal_text_notPID/    STM32 firmware without PID control
K210.py                K210 face detection and coordinate sender
README.md              English README
README.zh-CN.md        Chinese README
```

## Future Roadmap

### 1. Face Recognition

The current system detects whether a face exists, but it does not identify who the person is. Future versions will add face feature extraction and matching on the K210 or another edge AI module to support:

- face detection;
- authorized-person verification;
- `known face / unknown face` status output;
- result transmission to the STM32 or communication module.

### 2. LoRa Remote Reporting

A LoRa module will be added for long-range, low-power status reporting. The system will not transmit video; it will only send compact status messages such as:

```text
FACE_OK
FACE_UNKNOWN
NO_TARGET
DEVICE_ERROR
```

This makes the system suitable for a low-cost, low-power, long-range monitoring prototype.

### 3. Final Goal

```text
Face detection
  + Face recognition
  + Automatic pan-tilt tracking
  + LoRa remote status reporting
  = Low-cost intelligent monitoring system
```

## Notes

- This is an embedded prototype, not a finished security product.
- Face recognition and monitoring functions should comply with local laws and should be used only with proper authorization and consent.
- LoRa is suitable for low-rate status messages, not image or video transmission.
- For higher recognition accuracy, a better face detection or face recognition model may be required.
