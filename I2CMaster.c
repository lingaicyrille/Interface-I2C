#include <xc.h>
#include <stdio.h>
#include "LCD.h"

#pragma config FOSC=HSPLL
#pragma config WDTEN=OFF
#pragma config XINST=OFF

/*
Connections:
        Master RD5 <-> Slave RD5
        Master RD6 <-> Slave RD6
        2.2K pullups on both
        Dont forget GND <-> GND
        See Register Map for address and register details 
 */

void InitPins(void);
void ConfigInterrupts(void);
void ConfigPeriph(void);

unsigned char ReadWhoAmI(void);

void main(void) {
    long i;
    OSCTUNEbits.PLLEN = 1;
    LCDInit();
    LCDClear();
    InitPins();
    ConfigPeriph();
    ConfigInterrupts();
    lprintf(0, "I2C Master");
    unsigned char whoAmI = ReadWhoAmI();
    lprintf(1, "Device=0x%02x", whoAmI);
    while (1) {
        
        //Main loop here
    }
}

unsigned char ReadWhoAmI(void) {
    unsigned char rx;

    //First write register address
    SSP2CON2bits.SEN = 1; //Start condition
    while (SSP2CON2bits.SEN == 1); //Wait for start to finish
    SSP2BUF = 0xCC; //address with R/W clear for write
    while (SSP2STATbits.BF || SSP2STATbits.R_W); // wait until write cycle is complete
    //Could check for ACK here
    SSP2BUF = 0xfe; //WhoAmI register address
    while (SSP2STATbits.BF || SSP2STATbits.R_W);

    //Now read from the selected register
    SSP2CON2bits.RSEN = 1; //Repeated start
    while (SSP2CON2bits.RSEN == 1);
    SSP2BUF = 0xCD; //address with R/W set for read
    while (SSP2STATbits.BF || SSP2STATbits.R_W);
    SSP2CON2bits.RCEN = 1; // enable master for 1 byte reception
    while (!SSP2STATbits.BF); // wait until byte received
    rx = SSP2BUF; //Get the received byte
    SSP2CON2bits.ACKDT = 1; //NACK for last byte.  Use a 0 here to ACK
    SSP2CON2bits.ACKEN = 1; //Send ACK/NACK
    while (SSP2CON2bits.ACKEN != 0); //Wait for ACK/NACK to complete
    SSP2CON2bits.PEN = 1; //Stop condition
    while (SSP2CON2bits.PEN == 1); //Wait for stop to finish
    return rx;
}
unsigned short readTemp(){
    unsigned short Temp = 0x01;
  SSP2CON2bits.RSEN = 1; //Repeated start
    while (SSP2CON2bits.RSEN == 1);  
    Temp = readTemp(0xCD);
    Temp = readTemp(SSP2BUF);
    return Temp;
    
}
//add a function ReadPressure () that reads the pressure from the slave
unsigned short ReadPressure(){
    unsigned short PressL = 0x02;
    unsigned short PressH = 0x03;
    SSP2CON2bits.RSEN = 1; //Repeated start
    while (SSP2CON2bits.RSEN == 1);  
    SSP2BUF = 0xCD; //address with R/W set for read
    PressL = ReadPressure(SSP2BUF);
    PressH = PressL << 8;
    int Press = PressH + PressL;
    
    return Press;
}
void InitPins(void) {
    LATD = 0; //LED's are outputs
    TRISD = 0; //Turn off all LED's
    //Set TRIS bits for any required peripherals here.
    TRISB = 0b00000001; //Button0 is input;
    INTCON2bits.RBPU = 0; //enable weak pull ups on port B
    TRISD = 0b01100000; //MSSP2 uses RD5 as SDA, RD6 as SCL, both set as inputs

}

void ConfigInterrupts(void) {

    RCONbits.IPEN = 0; //no priorities.  This is the default.

    //Configure your interrupts here

    //set up INT0 to interrupt on falling edge
    INTCON2bits.INTEDG0 = 0; //interrupt on falling edge
    INTCONbits.INT0IE = 1; //Enable the interrupt
    //note that we don't need to set the priority because we disabled priorities (and INT0 is ALWAYS high priority when priorities are enabled.)
    INTCONbits.INT0IF = 0; //Always clear the flag before enabling interrupts
    INTCONbits.GIE = 1; //Turn on interrupts
}

void ConfigPeriph(void) {

    //Configure peripherals here
    SSP2STATbits.SMP = 0; //400kHz slew
    SSP2ADD = 19; //400kHz
    SSP2CON1bits.SSPM = 0b1000; //I2C Master mode
    SSP2CON1bits.SSPEN = 1; //Enable MSSP
}

void __interrupt(high_priority) HighIsr(void) {
    unsigned char rx;
    //Check the source of the interrupt
    if (INTCONbits.INT0IF == 1) {
        //source is INT0

        INTCONbits.INT0IF = 0; //clear the flag
    }
}


