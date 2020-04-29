#include "IR_module.h"

//Initialise TMR5
void TMR5_init(void){  //Initialise TMR5 registers. TMR5 used as a timer to calculate CAPxCON captures

  T5CONbits.T5SEN = 0; //Timer5 disabled during Sleep
  T5CONbits.RESEN = 0; //Special Event Reset disabled
  T5CONbits.T5MOD = 0; //Continuous Count mode enabled
  T5CONbits.T5PS = 0b11; //1:8 prescaler forces 250KHz on TMR5
  T5CONbits.T5SYNC = 0; //ignore bit
  T5CONbits.TMR5CS = 0; // use internal clock
  T5CONbits.TMR5ON = 1; //Enable tmr5

}

void IR_init(void){

  CAP1CONbits.CAP1REN = 1; //Enable time base reset
  CAP1CONbits.CAP1M = 0b0110; //Pulse Width Measurement mode, every falling to rising edge
  CAP2CONbits.CAP2REN = 1; //Enable time base reset
  CAP2CONbits.CAP2M = 0b0110; //Pulse Width Measurement mode, every falling to rising edge

  ANSEL0 = 0; //Disable analogue ports 1 and 0, making them digital
  ANSEL1 = 0;

}

//map signal received from the CAPCON registers. MAX is approx. 12500 accounting for the prescaled count of TMR5 at 50ms.
int map_infrared(int ir_signal) { 
     int map_signal = ir_signal/62; // //on the range of approx o to 200
     if (map_signal > 200) {map_signal = 200;} //reduce un-required noise above 200
     return map_signal;
}

//Collocate extracted mapped signal values into a structure saving the left and right signals 
void IR_signal_extract(struct IR_struct *ir){  
    
  ir->ir_left_old = ir->ir_left;
  ir->ir_right_old = ir->ir_right;
  ir->ir_left = (CAP2BUFH << 8) | (CAP2BUFL);  //Convert 8 bit BUFL and BUFH into 16 bits values for the left and right values
  ir->ir_right = (CAP1BUFH << 8) | (CAP1BUFL);

  if ((ir->ir_left == ir->ir_left_old) && (ir->ir_left <= 200)) { //if the values are identical to the previous and they are smaller than 200/12500, make them 0 to activate turn_left and re-search
  ir->ir_left = 0;}
  if ((ir->ir_right == ir->ir_right_old) && (ir->ir_right <= 200)) {
  ir->ir_right = 0;}

  ir->ir_left_mapped = map_infrared(ir->ir_left);
  ir->ir_right_mapped = map_infrared(ir->ir_right);
  
}

