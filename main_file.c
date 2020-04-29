#pragma config OSC = IRCIO, WDTEN = OFF, LVP=OFF, MCLRE=OFF //Internal oscillator,
#define _XTAL_FREQ 8000000  // Adjust frequency to ensure delay functions work accordingly 8MHZ

#include <stdio.h>
#include <stdlib.h>
#include <xc.h>

//We create functions in separate source files and add them with the header files to make the code neater
#include "IR_module2.h"
#include "LCD_module2.h"
#include "RFID_module2.h"
#include "Motor_module2.h"

//IR module contains functions which sets the IR registers, exracts the IR values and maps them from 0 to 200 in order to facilitate data manipulation.
//LCD module initialises the LCD and displays the values of the mapped infrared signals
//RFID module sets the RFID registers and contains functions that sends a clean version of the RFID ASCII data and performs the CHECKSUM. Success of checksum is displayed in the LCD
//Motor module sets TMR0 to count on external clock from the encoder, initialises motor functions and includes motor direction modalities.
//Motor module also has a function called store_back that stores the direcetions and distances when finding the RFID card

//3 distinct flags will be triggered during the while(1) loop. These flags are monitored by the variable card_read, which is altered by interrupts, therefore it must be a global variable

volatile char card_read; //value will change during the code at any time therefore it must be a volatile character
char string_rfid[17]; //global variable because it is used in the RFID interrupt and called in the main loop

//High priority interrupt sets card_read flags to 1 or 0 when the card is read by the RFID tag or the button is pressed respectively.
void interrupt InterruptHandlerHigh() {
    static char x = 0;
    char rx_char;

    if (PIR1bits.RCIF) { //if a character is read from the rfid tag
        rx_char = RCREG;
        if (rx_char == 0x02) {  //if it is the first character of the message
            x = 0;
        }
        if (rx_char == 0x03) {  //when all the message is read
            card_read =  1; //enter RFID reading and checksum functions
        }
        string_rfid[x] = rx_char; //store the character read by the RFID tag into a character string
        x++;
    }

    if (INTCON3bits.INT2IF) {   //external interrupt flag
        if (PORTCbits.RC5 == 1) {   //Ask 4 times for the RC5 input to prevent button false positives
            if (PORTCbits.RC5 == 1) {
                if (PORTCbits.RC5 == 1) {
                    if (PORTCbits.RC5 == 1) {
                        card_read = 0;  //re-start the searching routine
                    }
                }
            }
        }
        INTCON3bits.INT2IF = 0; //clear the interrupt flag
    }
}

void main(void) {

    OSCCON = 0x72; //internal oscillator, 8MHz
    while (!OSCCONbits.IOFS); //Wait for OSC to become stable

    int PWMcycle = 199; //Sets the base period of the motor PWM cycle

    //Setting up the IR structure
    struct IR_struct IR_values; // A structure for the IR values is created to ease access and storage of the IR_values

    IR_values.ir_left_mapped = 0;
    IR_values.ir_right_mapped = 0;
    IR_values.ir_left = 0;
    IR_values.ir_right = 0;
    IR_values.ir_left_old = 0;
    IR_values.ir_right_old = 0;


    //Setting up motor structure
    struct DC_motor motorL, motorR; //declare two DC_motor structures

    motorL.power = 0; //zero power to start
    motorL.direction = 1; //set default motor direction, forward
    motorL.dutyLowByte = (unsigned char *) (&PDC0L); //store address of PWM duty low byte
    motorL.dutyHighByte = (unsigned char *) (&PDC0H); //store address of PWM duty high byte
    motorL.dir_pin = 0; //pin RB0/PWM0 controls direction
    motorL.PWMperiod = PWMcycle; //store PWMperiod for motor

    //same for motorR but different PWM registers and direction pin
    motorR.power = 0; //zero power to start
    motorR.direction = 1; //set default motor direction, forward
    motorR.dutyLowByte = (unsigned char *) (&PDC1L); //store address of PWM duty low byte
    motorR.dutyHighByte = (unsigned char *) (&PDC1H); //store address of PWM duty high byte
    motorR.dir_pin = 2; //pin RB2/PWM0 controls direction
    motorR.PWMperiod = PWMcycle; //store PWMperiod for motor

    //Initialise module registers
    TMR0_init();
    TMR5_init();
    LCD_Init();
    IR_init();
    RFID_init();
    initPWM();
    TRISCbits.RC3 = 1; // Set the register for the quadrature encoder

    // INTERRUPT BITS
    RCREG;
    RCREG;
    RCREG; //Calling RCREG to clear any previous data
    PIE1bits.RCIE = 1; //Enable EUSART interrupt
    INTCONbits.GIEL = 1; //enable peripheral interrupts
    INTCONbits.GIEH = 1; //enable global interrupts
    INTCON3bits.INT2IE = 1; //enable external interrupts
    // END INTERRUPT BITS

    //Create loop variables

    char buf_r[16], buf_l[16]; //Array to display IR readings
    int difference = 0; //Used to compare the right and left IR readings: 'right - left'
    int threshold; //Variable to set a threshold for the value of  the variable "difference"

    //Speed levels for different commands
    char speed_motor_low = 53;
    char speed_motor_high = 72;

    //Arrays  storing the route to the IR emitter. Used for completing the return path.
    int back_trace_direction[60];
    int back_trace_distance[60];
    char direction = 0;
    int back_trace_counter = 0;

    //Arrays for checksum
    int bits_16[6];
    char array_check[12];

    //Start at rest
    card_read = 3; //Set card read flag to 3
    fullSpeedAhead(&motorL, &motorR, 1); //Required to implement the stop function while card_read == 3


//main while loop
    while (1) {

//Bring the motor to rest whilst the card_read flag value is unchanged
        if (card_read == 3) {
            stop(&motorL, &motorR);
        }


//Searching for IR emitter
        while (card_read == 0) { //Card read is set to 0 by the interrupt set by pressing the button

            IR_signal_extract(&IR_values); //Function extracting the mapped values for IR readings
            IR_LCD_display(buf_r, buf_l, &IR_values); //Function displaying the IR readings to check their sensitivity

            difference = IR_values.ir_right_mapped - IR_values.ir_left_mapped; //check the difference in the power of the read signals

            if ((IR_values.ir_right_mapped | IR_values.ir_left_mapped) > 180) { //If the signals are high
                threshold = 50; //Large threshold to prevent small fluctuations from affecting the direction choice
            } else { //if the signal are low
                threshold = 10; //Choice of threshold proportional to the signal reading
            }

            if (difference <= (-threshold)) { //if left signal > right signal
                if (direction != 1) { //if it wasn't turning left, stop to allow direction transition
                    stop(&motorL, &motorR);
                    back_trace_counter = store_back(&back_trace_direction[back_trace_counter], &back_trace_distance[back_trace_counter], direction, back_trace_counter); //Storing previous path and increases back trace counter
                }
                turnLeft(&motorL, &motorR, speed_motor_low);
                direction = 1; // set direction to 1 (left in the back trace array)
            }
            else if (difference >= threshold) { //if right signal > left signal
                if (direction != 2) {  //if it wasn't turning right, stop to allow direction transition
                    stop(&motorL, &motorR);
                    back_trace_counter = store_back(&back_trace_direction[back_trace_counter], &back_trace_distance[back_trace_counter], direction, back_trace_counter);
                }
                turnRight(&motorL, &motorR, speed_motor_low);
                direction = 2; // 2 is right in the back trace array, set direction to right

            }
            else {

                if ((IR_values.ir_right_mapped >= 140) && (IR_values.ir_left_mapped >= 140)) { //if the signal is high for both readers
                    if (direction != 3) { //if it wasn't going straight stop to change direction
                        stop(&motorL, &motorR);
                        back_trace_counter = store_back(&back_trace_direction[back_trace_counter], &back_trace_distance[back_trace_counter], direction, back_trace_counter);
                    }
                    fullSpeedAhead(&motorL, &motorR, speed_motor_low);
                    direction = 3; // 3 is forwards in the back trace array

                } else { //if the signal is too weak
                    if (direction != 1) { //if it is not going left stop to allow for direction transition
                        stop(&motorL, &motorR);
                        back_trace_counter = store_back(&back_trace_direction[back_trace_counter], &back_trace_distance[back_trace_counter], direction, back_trace_counter);
                    }
                    turnLeft(&motorL, &motorR, speed_motor_low); //This step makes the robot turn left when the signal is weak until the signals are strong, making it move forwards only in the right direction. This prevents the robot from getting lost.
                    direction = 1; // 1 is left in the back trace array
                }
            }
        }


//RFID reading, sending and checking
        if (card_read == 1) { //the interrupt for the RFID tag sets card_read to 1 when the card is read
            stop(&motorL, &motorR);

            //store last step of the route in the back_trace array
            back_trace_counter = store_back(&back_trace_direction[back_trace_counter], &back_trace_distance[back_trace_counter], direction, back_trace_counter);
            back_trace_counter--; //reduce back_trace_counter to the number of steps performed since store_back increases back trace counter
            send_signal_RFID(&string_rfid[0], &string_rfid[0]); //sends the significant characters read from the RFID to the LCD
            check_sum(&string_rfid[0], &array_check[0], &bits_16[0], back_trace_counter); //performs and displays checksum condition
            card_read = 2; //move to next loop
        }


//Back_trace operation starts and robot travells to its initial position
        if (card_read == 2) {

            for (back_trace_counter; back_trace_counter >= 0; back_trace_counter--) { // access only the values stored in the back_trace_array that have been iterated in the current test

                TMR0H = 0; // set TMROH and TMROL values to 0 every time back_trace_counter changes to re-start TMR0 count
                TMR0L = 0;

                while (((TMR0H << 8) | TMR0L) <= back_trace_distance[back_trace_counter]) { //compare the current TMR0 value read by the encoder to the previously stored value
                    if (back_trace_direction[back_trace_counter] == 1) { // if it was turning left, turn right in backtrace
                        turnRight(&motorL, &motorR, speed_motor_low);
                    } else if (back_trace_direction[back_trace_counter] == 2) { // if it was turning right, turn left in backtrace
                        turnLeft(&motorL, &motorR, speed_motor_low);
                    } else { // if it was going forwards, go backwards in backtrace
                        if (back_trace_distance[back_trace_counter] > 100) {  //if during the search it went straight for a long TMR0 value (distance), go back at high speed
                            fullSpeedBackwards(&motorL, &motorR, speed_motor_high);
                        } else { //else if the forward distance during the recorded step was low, go back slowly, in order to increase speed whilst reducing % error
                            fullSpeedBackwards(&motorL, &motorR, speed_motor_low);
                        }

                    }
                }
                stop(&motorL, &motorR); //present in order to allow direction transition
            }
            card_read = 3; //when all the back trace array has been accessed, exit loop and force the robot to rest
            back_trace_counter = 0; //reset back_trace_counter to zero (currently  - 1). This allows the robot to overwrite the back_trace_array when the button is pressed.
            direction = 0; //reset direction to zero to allow for direction reset during button press
        }
    }
}
