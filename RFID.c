#include "RFID_module.h"

void RFID_init(void) {
    TRISC = TRISC | 0b11000000; //set data direction registers to the TRGIF tag
    SPBRG = 204; //set baud rate to 9600
    SPBRGH = 0;
    BAUDCONbits.BRG16 = 1; //set baud rate scaling to 16 bit mode
    TXSTAbits.BRGH = 1; //high baud rate select bit
    RCSTAbits.CREN = 1; //continous receive mode
    RCSTAbits.SPEN = 1; //enable serial port, other settings default
    TXSTAbits.TXEN = 1; //enable transmitter, other settings default
}

//this functions inputs the address of the RFID string array. Values in the LCD displaying ir readings are cleared and the RFID string is sent to the LCD
void send_signal_RFID(char *pa, char *dis) {

    clear_LCD(); //Remove the current data displayed in the LCD
    SetLine(1);
    while (*pa != 0x03) { //Finds the address at the last RFID character
        pa++;
    }
    
    //Send ASCII characters read from the RFID string to the LCD
    while ((dis + 1) < (pa - 2)) { //dis is the address at the first value of the rfid string array. Starting at dis + 1 and finishing at pa - 2 removes undesired ASCII characters read by the RFID.

        SendLCD(*(dis + 1), 1); 
        __delay_us(50);

        dis++;
    }
    SetLine(2); // Set next line for the checksum display function
}


// This function calls the stored RFID string initial value address. The stores RFID string has 12 valuable characters in ASCII format. These are transformed into hexadecimal and stored in the array_check array. low and high bits are combined to form 16 bit integers in the bit_16 array, where the first 5 values are XORed and compared to the 6th value. If satisfied, checksum is correct.
void check_sum(char *str, char *arr_check, int *bit16, int btc) {

    //Conversion of the string from ASCII to Hexadecimal
    int i = 0;
    while (i < 12) { // store the 12 valuable characters of the RFID called with the *str address into the array_check array
        if ((*(str + 1) >> 6) == 1) {   //If the (str + 1)th character in the RFID string is an ASCII letter, subtract 55 and move to the next character
            *(arr_check) = *(str + 1) - 55;
            str++; 
            arr_check++;
            i++;
        } else { //else if it is an ASCII number, subtract 48 and move to the next character
            *(arr_check) = *(str + 1) - 48;
            str++;
            arr_check++;
            i++;
        }
    }
    
    for (char j = 0; j < 6; j++) {  //bit_16(count) is written with overlaid low and high bits from the array_check array, compressing a 12 dimensional array into a 6 dimensional array 
        *bit16 = (*(arr_check + 2*j - 12) << 8) | *(arr_check + 2*j + 1 - 12);  //previous arr_check stopped at 12th position, thus - 12 is necessary to start at the first position of array_check
        bit16++; //move to the next address in the bit_16 array
    }

    int xor_value = ((*(bit16 - 6))^(*(bit16 - 5))^(*(bit16 - 4))^(*(bit16 - 3))^(*(bit16 - 2))); // calculate the XOR value of the first 5 entries of the bit_16 array (starts at -6 instead of -5 because the bit16 address is increased in the last iteration above).
    
    char buf_cs[16];
    if (xor_value == *(bit16-1)) { //check if the calculated XOR value is identical to the RFID string checksum value and display condition in the LCD display
        sprintf(buf_cs, "CS CORRECT %d", btc); //Also displaying the value of back trace counter to see the number of steps
        LCD_string(buf_cs);
    } else {
        sprintf(buf_cs, "CS INCORRECT %d", btc);
        LCD_string(buf_cs);
    }

}




