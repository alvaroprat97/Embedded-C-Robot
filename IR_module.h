#ifndef _IR_H
#define _IR_H

#include <xc.h>
#define _XTAL_FREQ 8000000 //define _XTAL_FREQ so delay routines work

struct IR_struct {
    int ir_left_mapped;
    int ir_right_mapped;
    int ir_left;
    int ir_right;
    int ir_left_old;
    int ir_right_old;
};

//Refer to Motor.c for function description
void TMR5_init(void);
void IR_init(void);
int map_infrared(int ir_signal);
void IR_signal_extract(struct IR_struct *ir);
#endif