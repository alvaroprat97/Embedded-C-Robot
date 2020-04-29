#include "LCD_module.h"

//function to togle enable bit on then off for 5 microseconds as required by the LCD display
void E_TOG(void){
LATCbits.LATC0 = 1;
__delay_us(5); 
LATCbits.LATC0 = 0;
}

//function to send the four bits to the LCD, sent as digital signals for 5 microseconds.
void LCDout(unsigned char number){
//set data pins using the four bits from number
//toggle the enable bit to send data
LATC =  ((LATC & 0b11111001) | ((number & 0b00000011) << 1));
LATD =  ((LATD & 0b11111100) | ((number & 0b00001100) >> 2));
E_TOG(); //turns on and off the LATC signals 
__delay_us(5); // 5us delay
}

//function to send data/commands over a 4bit interface
void SendLCD(unsigned char Byte, int type){  
 LATAbits.LATA6 = type; // set RS pin whether it is a Command (0) or Data/Char (1)
 LCDout(Byte >> 4); //send the 4 high bits of the byte
 __delay_us(10); // 10us delay
 LCDout(Byte & 0b00001111); //send the low 4 bits of the byte
 __delay_us(50); // 50us delay
}

//This function sets the LCD pin registers and perform the LCD initialisation code as predefined in the data sheet
void LCD_Init(void){
    
// set initial LAT output values (they start up in a random state)
    LATA=0;
    LATC=0;
    LATD=0;
//Initialise the pins from the PIC where the LCD is connected to as outputs
    TRISAbits.RA6 = 0;
    TRISCbits.RC0 = 0;
    TRISCbits.RC1 = 0;
    TRISCbits.RC2 = 0;
    TRISDbits.RD0 = 0;
    TRISDbits.RD1 = 0;

 // Initialisation sequence code as defined in the data sheet
    __delay_ms(15); 
    LCDout(0b0011);
    __delay_ms(5);  
    LCDout(0b0011);
    __delay_us(200);   
    LCDout(0b0011);
    __delay_us(50);     
    LCDout(0b0010);
    __delay_us(50);
    SendLCD(0b00101000, 0); //function set
    __delay_us(50);
    SendLCD(0b00001000, 0); // display off
    __delay_us(50);
    SendLCD(0b00000001, 0); //display clear
    __delay_ms(2);
    SendLCD(0b00000110, 0); //entry mode
    __delay_us(50);
    SendLCD(0b00001110, 0); // display on with cursor
    __delay_us(50);
}

//function to set the cursor at the beginning of either the first or the second line
void SetLine (int line){
    if (line == 1) {
//Send 0x80 to set line to 1 (0x00 ddram address)
    SendLCD(0x80, 0);}
    if (line == 2) {
//Send 0xC0 to set line to 2 (0x40 ddram address)
    SendLCD(0xC0, 0);}
 __delay_us(50); 
}

//function uses a pointer to output a string of characters to the LCD screen
void LCD_string(char *string) {
    //while the data pointed to isn't 0x00 do below (0x00 is the last character in the character string)
    while (*string != 0x00) {
        //send out the current byte pointed
        //and increment the pointer
        SendLCD(*string++, 1);
        __delay_us(50); //so we can see each character
        //being printed in turn (remove delay to display instantly)
    }
}

//Clear the values displayed in the LCD
void clear_LCD(void){
    SendLCD(0b00000001,0);
    __delay_ms(2);
}

//Function displays the updating mapped infrared values in the LCD and the TMR0 values from the quadrature encoder. 
//Used for debugging and calibration purposes.
void IR_LCD_display(char *buf_r, char *buf_l, struct IR_struct *ir){
     
     clear_LCD(); //Clear previous IR values displayed in the LCD
     
     sprintf(buf_r,"PR = %d %02d",ir->ir_right_mapped, TMR0L); //Writes the the left and right mapped IR signals and the updating encoder TMR0 Low and High values into the buf_r and buf_l address
     sprintf(buf_l,"PL = %d %02d",ir->ir_left_mapped, TMR0H);

     SetLine(1);
     LCD_string(buf_r);
     SetLine(2);
     LCD_string(buf_l);

     __delay_ms(50);
     __delay_ms(50);
     __delay_ms(50);
     __delay_ms(50);

}
