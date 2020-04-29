#ifndef _RFID_H
#define _RFID_H

#include <xc.h>
#include "LCD_module.h"
#define _XTAL_FREQ 8000000 //define _XTAL_FREQ so delay routines work

//Refer to RFID.c for function description
void RFID_init(void);
void send_signal_RFID(char *pa,char *dis);
void check_sum(char *str, char *arr_check, int *bit16, int btc);

#endif