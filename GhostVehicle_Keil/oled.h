#ifndef OLED_H
#define OLED_H
#include <stdint.h>

void    OLED_Init(void);
uint8_t OLED_Probe(void);                       /* 1 = OLED answered on I2C, 0 = no ACK */
void    OLED_Clear(void);
void    OLED_SetCursor(uint8_t page, uint8_t col);
void    OLED_PrintChar(char c);
void    OLED_PrintString(const char *str);
void    OLED_PrintNumber(uint16_t number);
void    OLED_PrintLine(uint8_t page, const char *str);  /* prints str, pads to 21 columns, no flicker */

#endif
