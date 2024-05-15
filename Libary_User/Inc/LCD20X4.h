#ifndef __LCD20X4_H_
#define __LCD20X4_H_

#include "stm32f1xx_hal.h"

#define LCD_RS GPIO_PIN_12
#define LCD_RS_PORT GPIOB

#define LCD_EN GPIO_PIN_13
#define LCD_EN_PORT GPIOB

#define LCD_D4 GPIO_PIN_14
#define LCD_D4_PORT GPIOB

#define LCD_D5 GPIO_PIN_15
#define LCD_D5_PORT GPIOB

#define LCD_D6 GPIO_PIN_8
#define LCD_D6_PORT GPIOA

#define LCD_D7 GPIO_PIN_9
#define LCD_D7_PORT GPIOA

void LCD20x4_Enable(void);

void LCD20X4_Send4Bit(uint8_t Data);

void LCD20X4_SendCommand(uint8_t Command, uint8_t reg);

void LCD20X4_Clear(void);

void LCD20X4_Init(void);

void LCD20X4_Gotoxy(uint8_t x, uint8_t y);

void LCD20X4_PutChar(char Data);

void LCD20X4_PutString(char *s);

void LCD20X4_SendInteger(int Number);

void LCD20X4_SendFloat(float Number);

#endif
