//
// Created by 35400 on 2026/3/3.
// OLED 8x16 font definition
//

#ifndef GIMBAL_V1_OLED_FONT_H
#define GIMBAL_V1_OLED_FONT_H

#include <stdint.h>

#define OLED_FONT_WIDTH  8
#define OLED_FONT_HEIGHT 16
#define OLED_FONT_COUNT  95

extern const uint8_t OLED_F8x16[OLED_FONT_COUNT][OLED_FONT_HEIGHT];

#endif // GIMBAL_V1_OLED_FONT_H