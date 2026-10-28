#include "TM4C123GH6PM.h"
#include "oled.h"
#include "font.h"
#include "delay.h"

#define OLED_ADDR 0x3C     /* SSD1306 7-bit I2C address */
#define OLED_COLS 21       /* 21 characters x 6 pixels = 126 pixels per line */

static void OLED_Command(uint8_t cmd) {
    I2C0->MSA = (OLED_ADDR << 1);
    I2C0->MDR = 0x00;                 /* control byte: command */
    I2C0->MCS = 0x03;                 /* START + RUN, no stop yet */
    while (I2C0->MCS & 0x01);
    I2C0->MDR = cmd;
    I2C0->MCS = 0x05;                 /* RUN + STOP */
    while (I2C0->MCS & 0x01);
}

static void OLED_Data(uint8_t data) {
    I2C0->MSA = (OLED_ADDR << 1);
    I2C0->MDR = 0x40;                 /* control byte: data */
    I2C0->MCS = 0x03;
    while (I2C0->MCS & 0x01);
    I2C0->MDR = data;
    I2C0->MCS = 0x05;
    while (I2C0->MCS & 0x01);
}

/* Address-only style check: send one control byte and look at the ERROR bit.
   Use it to tell a wiring problem (no ACK) from a software problem. */
uint8_t OLED_Probe(void) {
    I2C0->MSA = (OLED_ADDR << 1);
    I2C0->MDR = 0x00;
    I2C0->MCS = 0x07;                 /* START + RUN + STOP */
    while (I2C0->MCS & 0x01);
    return (I2C0->MCS & 0x02) ? 0 : 1;
}

void OLED_Init(void) {
    Delay_ms(100);                    /* let the panel power up */
    OLED_Command(0xAE);               /* display off */
    OLED_Command(0xD5); OLED_Command(0x80);
    OLED_Command(0xA8); OLED_Command(0x3F);
    OLED_Command(0xD3); OLED_Command(0x00);
    OLED_Command(0x40);
    OLED_Command(0x8D); OLED_Command(0x14);   /* charge pump on */
    OLED_Command(0x20); OLED_Command(0x00);   /* horizontal addressing */
    OLED_Command(0xA1);               /* segment remap */
    OLED_Command(0xC8);               /* COM scan direction */
    OLED_Command(0xDA); OLED_Command(0x12);
    OLED_Command(0x81); OLED_Command(0xCF);
    OLED_Command(0xD9); OLED_Command(0xF1);
    OLED_Command(0xDB); OLED_Command(0x40);
    OLED_Command(0xA4);
    OLED_Command(0xA6);
    OLED_Command(0xAF);               /* display ON */
}

void OLED_SetCursor(uint8_t page, uint8_t col) {
    OLED_Command(0xB0 + page);
    OLED_Command(0x00 + (col & 0x0F));
    OLED_Command(0x10 + ((col >> 4) & 0x0F));
}

void OLED_Clear(void) {
    uint8_t page, col;
    for (page = 0; page < 8; page++) {
        OLED_SetCursor(page, 0);
        for (col = 0; col < 128; col++)
            OLED_Data(0x00);
    }
}

void OLED_PrintChar(char c) {
    uint8_t index, i;
    if (c >= 'a' && c <= 'z') c -= 32;        /* font table has capitals only (ASCII 32-95) */
    if (c < 32 || c > 95) c = '?';            /* never read outside the table */
    index = (uint8_t)(c - 32);
    for (i = 0; i < 5; i++)
        OLED_Data(font5x7[index][i]);
    OLED_Data(0x00);                          /* 1 pixel spacing */
}

void OLED_PrintString(const char *str) {
    while (*str) OLED_PrintChar(*str++);
}

void OLED_PrintNumber(uint16_t number) {
    char buf[6];
    int i = 0;
    if (number == 0) { OLED_PrintChar('0'); return; }
    while (number > 0) { buf[i++] = (char)((number % 10) + '0'); number /= 10; }
    while (i > 0) OLED_PrintChar(buf[--i]);
}

void OLED_PrintLine(uint8_t page, const char *str) {
    uint8_t n = 0;
    OLED_SetCursor(page, 0);
    while (*str && n < OLED_COLS) { OLED_PrintChar(*str++); n++; }
    while (n < OLED_COLS)         { OLED_PrintChar(' ');    n++; }
}
