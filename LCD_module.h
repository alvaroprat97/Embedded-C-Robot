#ifndef _LCD_H
#define _LCD_H

#include <xc.h>
#include <stdio.h>
#include "IR_module.h"
#define _XTAL_FREQ 8000000 //define _XTAL_FREQ so delay routines work

//Refer to LCD.c for function description
void E_TOG(void);
void LCDout(unsigned char number);
void SendLCD(unsigned char Byte, int type);
void LCD_Init(void);
void SetLine (int line);
void LCD_string(char *string);
void clear_LCD(void);
void IR_LCD_display(char *buf_r, char *buf_l, struct IR_struct *ir);
#endif