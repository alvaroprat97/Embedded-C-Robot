#ifndef _MOTOR_H
#define _MOTOR_H

#include <xc.h>
#define _XTAL_FREQ 8000000 //define _XTAL_FREQ so delay routines work

struct DC_motor {
    char power; //motor power, out of 100, plays the role of ptmr
    char direction; //motor direction, forward(0), reverse(1)
    unsigned char *dutyLowByte; //PWM duty low byte address
    unsigned char *dutyHighByte; //PWM duty high byte address
    char dir_pin; // pin that controls direction on PORTB
    int PWMperiod; //base period of PWM cycle
};

//Refer to Motor.c for function description
void TMR0_init(void);
void setMotorPWM(struct DC_motor *m);
void setMotorFullSpeed(struct DC_motor *m, char max_p);
void initPWM(void);
void stop(struct DC_motor *m_L, struct DC_motor *m_R);
void turnLeft(struct DC_motor *m_L, struct DC_motor *m_R, char speed);
void turnRight(struct DC_motor *m_L, struct DC_motor *m_R, char speed);
void forwards(struct DC_motor *m_L, struct DC_motor *m_R, char speed);
void backwards(struct DC_motor *m_L, struct DC_motor *m_R, char speed);
int store_back(int *back_trace_dir, int *back_trace_dist, char dir, int btc);
#endif