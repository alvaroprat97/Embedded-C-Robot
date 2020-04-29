#include "Motor_module.h"

//Enable TMR0 as in external counting mode. TOCKI pin will increment the value of TMR0 every 4 low to high transitions. 
//This will be used to store TMR0L and TMR0H values in the back_trace_array through the quadrature encoder.
//The prescaler is used to be conservative and prevent too rapid overflow.
void TMR0_init(void) {
    T0CONbits.TMR0ON = 1; //turn on timer0
    T0CONbits.T016BIT = 0; // 16bit mode
    T0CONbits.T0CS = 1; // Transition on T0CKI pin input edge
    T0CONbits.PSA = 0; //Enable prescaler
    T0CONbits.T0PS = 0b001; // set prescaler value to 4
    T0CONbits.T0SE = 0; // Increment on low-to-high transition on T0CKI pin
    TMR0L = 0; //Initialise TMR0L and TMR0H to 0, these are re-set to 0 every time the back_trace_counter step increases
    TMR0H = 0; 
}

//sets the motor speed and direction to the motor
void setMotorPWM(struct DC_motor *m) {
    int PWMduty; //temporary variable to store PWM duty cycle

    if ((m->direction) == 1) { //if forwards
        // low voltage time increases with power
        PWMduty = m->PWMperiod - ((int) (m->power)*(m->PWMperiod)) / 100; //0 if power is max (max power is 100)
    } else { //if backwards
        // high voltage time increases with power
        PWMduty = ((int) (m->power)*(m->PWMperiod)) / 100; // 199 if power is max
    }
    
    PWMduty = (PWMduty << 2); // two LSBs are reserved for other things
    *(m->dutyLowByte) = PWMduty & 0xFF; //set low duty cycle byte
    *(m->dutyHighByte) = (PWMduty >> 8) & 0x3F; //set high duty cycle byte

    if (m->direction == 1) { // if direction is high
        LATB = LATB | (1 << (m->dir_pin)); // set dir_pin bit in LATB to high
    } else { // if direction is low,
        LATB = LATB & (~(1 << (m->dir_pin))); // set dir_pin bit in LATB to low
    }
}

//function inputs the motor structure address. A structure pointer *m allocates the variable values in the structure. This function sets the power value in the motor to a selected input max_p ( 0 to 100 )
void setMotorFullSpeed(struct DC_motor *m, char max_p) {
    for (m->power; m->power < max_p; m->power++) { //increase motor power until max_p
        setMotorPWM(m); //pass pointer to setMotorSpeed function 
    }
}

// function to initialise the PWM. Period is set to 20ms, PTMRPS presecaler is set to 1, therefore to achieve 20ms, PTPER is set to 99 
void initPWM(void) { 
    TRISB = 0;
    PTCON0 = 0b00000000; // free running mode, 1:1 pre scaler
    PTCON1 = 0b10000000; // enable PWM timer
    PWMCON0 = 0b01101111; // PWM1/3 enabled, all independent mode
    PWMCON1 = 0x00; // special features, all 0 (default)
    PTPERL = 0b11000111; // base PWM period low byte
    PTPERH = 0b00000000; // base PWM period high byte
}

//function brings power on both left and right motors to 0
void stop(struct DC_motor *m_L, struct DC_motor *m_R) {
    for (m_R->power; m_R->power >= 1; m_R->power--) //not stopping directly to prevent changing LATB values repetitively
    {
        setMotorPWM(m_R); //when power will be max, PWMduty will drop to 0
        __delay_us(50);
    }
    for (m_L->power; m_L->power >= 1; m_L->power--) //same as for R
    {
        setMotorPWM(m_L);
        __delay_us(50);
    }
}

// turn robot base left on the spot by setting the directions of the left and right motors to be back and forwards respectively
void turnLeft(struct DC_motor *m_L, struct DC_motor *m_R, char speed) {
    (m_L->direction) = 0; //Left motor goes backwards
    (m_R->direction) = 1; //Right motor goes forward
    setMotorFullSpeed(m_L, speed); //full speed on motor left backwards
    setMotorFullSpeed(m_R, speed); //full speed on motor left forwards
}

// turn robot base right on the spot
void turnRight(struct DC_motor *m_L, struct DC_motor *m_R, char speed) {
    (m_L->direction) = 1; //Left motor goes forward
    (m_R->direction) = 0; //Right motor goes backwards
    setMotorFullSpeed(m_L, speed); //full speed on motor left forwards
    setMotorFullSpeed(m_R, speed); //full speed on motor right backwards
}

// both motors forwards
void forwards(struct DC_motor *m_L, struct DC_motor *m_R, char speed) {
    (m_L->direction) = 1; //Left motor goes forward
    (m_R->direction) = 1; //Right motor goes forward
    setMotorFullSpeed(m_L, speed); //full speed on motor left forwards
    setMotorFullSpeed(m_R, speed); //full speed on motor right forwards
}

//set both motors backwards (only used during back_trace commands where forward is replaced for backward movements)
void backwards(struct DC_motor *m_L, struct DC_motor *m_R, char speed) {
    (m_L->direction) = 0; //Left motor goes forward
    (m_R->direction) = 0; //Right motor goes forward
    setMotorFullSpeed(m_L, speed); //full speed on motor left backwards
    setMotorFullSpeed(m_R, speed); //full speed on motor right backwards
}

//Store Back function is called when there is a change in direction. The function stores the direction and distance covered in the previous movement and increases the step counter by one.
int store_back(int *back_trace_dir, int *back_trace_dist, char dir, int btc) {
    if (dir != 0) {
        *back_trace_dir = dir; //*back_trace_dir will be the address of the bac_trace_direction array at the back_trace_counter position. Its value is assigned with the dir input.
        *back_trace_dist = ((TMR0H << 8) | TMR0L); //similarly, *back_trace_dist is a pointer that indicates the address of the back_trace_distance at the current step position. Its value is assigned with the TMR0 values extracted from the quadrature encoder.
        btc++; //Move to next step
        TMR0H = 0; //TMRO values are reset to 0 in order to start counting the distance covered in the following movement, with a reference of 0.
        TMR0L = 0;
    }
    return btc; //function returns back_trace_counter (btc). This is done in order to reduce the need of setting a global variable, since back_trace_counter is reduced by 1 when the card is read and re-set to 0 when the back_trace command is complete
}