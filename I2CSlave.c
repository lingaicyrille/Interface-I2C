#include <xc.h>
#include <stdio.h>
#include "LCD.h"
#include <stdlib.h>

#pragma config FOSC=HSPLL
#pragma config WDTEN=OFF
#pragma config XINST=OFF

/*
Connections:
    Master RD5 <-> Slave RD5
    Master RD6 <-> Slave RD6
    2.2K pullups on both
 */


void InitPins(void);
void ConfigInterrupts(void);
void ConfigPeriph(void);
unsigned int ReadPot(void);

char rx;
signed char temperature = 20;
int pressure;
unsigned char sampleRate = 5;
unsigned char currentRegister;
int bufferedPressure;
int state = 0;

void main(void) {
    OSCTUNEbits.PLLEN = 1;
    LCDInit();
    LCDClear();
    InitPins();
    ConfigPeriph();
    ConfigInterrupts();
    srand(ReadPot());
    lprintf(0, "I2C Slave");

    while (1) {
        unsigned int r = rand();
        r %= 5;
        r -= 2;
        temperature += r;
        if (temperature > 100) {
            temperature = 100;
        } else if (temperature < 0) {
            temperature = 0;
        }
        pressure = ReadPot();
        INTCONbits.GIE = 1;
        lprintf(1, "T:%d P:%u", temperature, pressure);
        LATDbits.LATD0 ^= 1;
        for (int i = 0; i < sampleRate; ++i) {
            __delay_ms(100);
        }
    }
}

void InitPins(void) {
    LATD = 0; //LED's are outputs
    TRISD = 0b01100000; //MMSP2 uses RD5 as SDA, RD6 as SCL, both set as inputs

    //Set up for ADC
    TRISA = 0b00000001;
    ADCON1 = 0b10111010; //Right justify, No calibration, 20 Tad, FOSC/32
    WDTCONbits.ADSHR = 1; //Switch to alternate address to access ANCON0
    ANCON0 = 0b11111110; //AN0 analog - all others digital
    WDTCONbits.ADSHR = 0; //Back to default address
}

void ConfigInterrupts(void) {

    RCONbits.IPEN = 0; //no priorities.  This is the default.

    //Enable MSSP interrupt
    PIE3bits.SSP2IE = 1;

    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1; //Turn on interrupts
}

void ConfigPeriph(void) {

    //Configure peripherals here

    SSP2ADD = 0xCC; //Slave address
    SSP2CON2bits.SEN = 1; //Enable clock stretching
    SSP2CON1bits.SSPM = 0b0110; //I2C Slave mode - don't interrupt on Start/Stop
    SSP2CON1bits.SSPEN = 1; //Enable MSSP
}

void __interrupt(high_priority) HighIsr(void) {
    //Check the source of the interrupt
    if (PIR3bits.SSP2IF == 1) {
        PIR3bits.SSP2IF = 0;
        rx = SSP2BUF; //always read the byte - this clears the BF bit
        if (SSP2STATbits.R_W == 0) //0 == Write, 1 == Read
        {
            //Write to slave
            if (SSP2STATbits.D_A == 1) //1 == Data, 0 == Address
            {
                //The received byte is data, not address
                if (state == 0) {
                    //First byte after address is register number
                    currentRegister = rx;
                    state = 1;
                } else if ((state == 1) && (currentRegister == 0x04)) {
                    //Only register 0x04 is writable
                    sampleRate = rx;
                } 
            } else { //Address byte
                state = 0;  //set state back to 0 when we get a write address
            }
        } else {  
            //Read from slave
            if (currentRegister == 0x01) {
                SSP2BUF = temperature;
            } else if (currentRegister == 0x02) {
                bufferedPressure = pressure;
                SSP2BUF = bufferedPressure;
                ++currentRegister;  //increment register since it is a 2 byte value
            } else if (currentRegister == 0x03) {
                SSP2BUF = bufferedPressure >> 8;
            } else if (currentRegister == 0x04) {
                SSP2BUF = sampleRate;
            } else if (currentRegister == 0xfe) {
                SSP2BUF = 0x73;
            } else {
                SSP2BUF = 0xff;  //Invalid register read
            }
        }
        SSP2CON1bits.CKP = 1; //Always release the clock - we're done with this request
        PIR3bits.SSP2IF = 0;  //Clear the interrupt flag
    }
}

unsigned int ReadPot(void) {
    unsigned int value;
    ADCON0bits.CHS = 0; //channel 0
    ADCON0bits.ADON = 1;
    ADCON0bits.GO = 1;
    while (ADCON0bits.GO == 1);
    ADCON0bits.ADON = 0;
    INTCONbits.GIE = 0;
    value = ADRES;
    INTCONbits.GIE = 1;
    return value;
}


