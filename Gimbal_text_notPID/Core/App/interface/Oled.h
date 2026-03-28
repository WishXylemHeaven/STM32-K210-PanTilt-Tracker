
#ifndef GIMBAL_V1_OLED_H
#define GIMBAL_V1_OLED_H

#include "Types/bsp_system.h"

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ClearLine(uint8_t Line);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_PrintfSetPos(uint8_t Line, uint8_t Column);


#endif //GIMBAL_V1_OLED_H