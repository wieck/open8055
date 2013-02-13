/********************************************************************
 FileName:     	HardwareProfile_Board1.h
 Dependencies:  See INCLUDES section
 Processor:     PIC18F type USB Microcontroller
 Hardware:      Open8055 based on Vellaman K8055-1
 Compiler:      Microchip C18
 Company:       Microchip Technology, Inc.

 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the “Company”) for its PIC® Microcontroller is intended and
 supplied to you, the Company’s customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

********************************************************************
 File Description:

 Change History:
  Rev   Date         Description
  1.0   11/19/2004   Initial release
  2.1   02/26/2007   Updated for simplicity and to use common
                     coding style
  2.3   09/15/2008   Broke out each hardware platform into its own
                     "HardwareProfile - xxx.h" file
                     
  Open8055
  
  0.1	11/13/2012	Adjusted for Open8055 Firmware on modified
  					K8055 board
********************************************************************/

#ifndef HARDWARE_PROFILE_BOARD1_H
#define HARDWARE_PROFILE_BOARD1_H

    /** SELF POWER *****************************************************/

    #define tris_usb_bus_sense  
    #define USB_BUS_SENSE       1
    #define tris_self_power     
    #define self_power          1
    
    /** IO PORT SETTINGS ***********************************************/

    //Port A
    //	RA0, RA1 are analog inputs
    //	RA2, RA3 are the board addres jumpers sk5, sk6
    //	RA4, RA5 are digital inputs I1, I2
    #define OPEN8055_TRISA 0x3F
    
    //Port B
    //	RB0..RB7 are digital outputs D1..D8
    #define OPEN8055_TRISB 0x00
    
    //Port C
    //	RC0 is digital input I3
    //	RC1, RC2 are PWM outputs
    //	RC6, RC7 are digital inputs I4, I5
    #define OPEN8055_TRISC 0xC9

    /** SWITCHES *******************************************************/

    #define OPEN8055sw1	PORTAbits.RA4
    #define OPEN8055sw2	PORTAbits.RA5
    #define OPEN8055sw3	PORTCbits.RC0
    #define OPEN8055sw4	PORTCbits.RC6
    #define OPEN8055sw5	PORTCbits.RC7

    /** ADDRESS JUMPER *************************************************/

    #define OPEN8055sk5	PORTAbits.RA2
    #define OPEN8055sk6	PORTAbits.RA3
    
    /** OUPUT PORTS ****************************************************/

    #define OPEN8055d1	PORTBbits.RB0
    #define OPEN8055d2	PORTBbits.RB1
    #define OPEN8055d3	PORTBbits.RB2
    #define OPEN8055d4	PORTBbits.RB3
    #define OPEN8055d5	PORTBbits.RB4
    #define OPEN8055d6	PORTBbits.RB5
    #define OPEN8055d7	PORTBbits.RB6
    #define OPEN8055d8	PORTBbits.RB7
    
    /** ADC CONFIG *****************************************************/

    // ADC config is in the CPU specific profiles.

    /** PWM CONFIG *****************************************************/
	
    //The original K8055 PWM runs at 23.43 kHz. 
    //At 48 MHz Fosc with a Timer2 prescale of 4 
    // 1 / ((127+1) * 4 * (1/Fosc) * 4) = 23,437
    // But that only leaves us with 9 bit resolution.
    // We rather have 10 bits and run at 46.86 kHz.
    // 1 / ((255+1) * 4 * (1/Fosc) * 1) = 48,875
    #define OPEN8055_T2CKPS0	0		// Timer2 prescaler = 1
    #define OPEN8055_T2CKPS1	0
    #define OPEN8055_PWMPR2		255		// PWMPR2 = 255
    #define OPEN8055_CCP1CON	0x0C
    #define OPEN8055_CCP2CON	0x0C
    
#endif  //HARDWARE_PROFILE_BOARD1_H
