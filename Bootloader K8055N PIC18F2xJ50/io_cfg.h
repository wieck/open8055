/*********************************************************************
 *
 *                Microchip USB C18 Firmware 
 *
 *********************************************************************
 * FileName:        io_cfg.h
 * Dependencies:    See INCLUDES section below
 * Processor:       PIC18
 * Compiler:        C18 3.11+
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the “Company”) for its PICmicro® Microcontroller is intended and
 * supplied to you, the Company’s customer, for use solely and
 * exclusively on Microchip PICmicro Microcontroller products. The
 * software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 * File Version  Date		Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 1.0			 04/09/2008	Started from MCHPFSUSB v1.3 HID Mouse
 *							demo project.  Commented out items that
 *							are not particularly useful for the
 *							bootloader.
 * Unofficial	02/17/2012	Added IO port definition section for the
 *							K8055_PIC18F2550_Bootloader.
 ********************************************************************/

#ifndef IO_CFG_H
#define IO_CFG_H

/** I N C L U D E S *************************************************/
#include "usbcfg.h"

/** T R I S *********************************************************/
#define INPUT_PIN           1
#define OUTPUT_PIN          0

#if defined(K8055N_PIC18F24J50)
    /** SELF POWER *****************************************************/

    #define tris_usb_bus_sense  
    #define usb_bus_sense       1
    #define tris_self_power     
    #define self_power          1

    /** LED ************************************************************/

    #define mInitAllLEDs()      TRISB = 0x00; LATB = 0x00;

    #define mLED_1              LATBbits.LATB0
    #define mLED_2              LATBbits.LATB1
    #define mLED_3              LATBbits.LATB2
    #define mLED_4              LATBbits.LATB3
    #define mLED_5              LATBbits.LATB4
    #define mLED_6              LATBbits.LATB5
    #define mLED_7              LATBbits.LATB6
    #define mLED_8              LATBbits.LATB7
    
    #define mLED_1_On()         mLED_1 = 1;
    #define mLED_2_On()         mLED_2 = 1;
    #define mLED_3_On()         mLED_3 = 1;
    #define mLED_4_On()         mLED_4 = 1;
    #define mLED_5_On()         mLED_5 = 1;
    #define mLED_6_On()         mLED_6 = 1;
    #define mLED_7_On()         mLED_7 = 1;
    #define mLED_8_On()         mLED_8 = 1;
    
    #define mLED_1_Off()        mLED_1 = 0;
    #define mLED_2_Off()        mLED_2 = 0;
    #define mLED_3_Off()        mLED_3 = 0;
    #define mLED_4_Off()        mLED_4 = 0;
    #define mLED_5_Off()        mLED_5 = 0;
    #define mLED_6_Off()        mLED_6 = 0;
    #define mLED_7_Off()        mLED_7 = 0;
    #define mLED_8_Off()        mLED_8 = 0;
    
    #define mLED_1_Toggle()     mLED_1 = !mLED_1;
    #define mLED_2_Toggle()     mLED_2 = !mLED_2;
    #define mLED_3_Toggle()     mLED_3 = !mLED_2;
    #define mLED_4_Toggle()     mLED_4 = !mLED_2;
    #define mLED_5_Toggle()     mLED_5 = !mLED_2;
    #define mLED_6_Toggle()     mLED_6 = !mLED_2;
    #define mLED_7_Toggle()     mLED_7 = !mLED_2;
    #define mLED_8_Toggle()     mLED_8 = !mLED_2;

    /** SWITCH *********************************************************/

	#define mInitAllSwitches()  TRISAbits.TRISA3 = 1; TRISAbits.TRISA5 = 1; \
								TRISCbits.TRISC0 = 1; TRISCbits.TRISC7 = 1; \
								TRISCbits.TRISC6 = 1;

	#define sw1                 PORTAbits.RA3
	#define sw2                 PORTAbits.RA5
	#define sw3                 PORTCbits.RC0
	#define sw4                 PORTCbits.RC6
	#define sw5                 PORTCbits.RC7

    /** ADDRESS JUMPER *************************************************/

    #define mInitAllJumpers()	TRISAbits.TRISA2 = 0; \
    							TRISCbits.TRISC1 = 1; TRISCbits.TRISC2 = 1;
	
	#define sk56power			PORTAbits.RA2
    #define sk5                 PORTCbits.RC1
    #define sk6                 PORTCbits.RC2

    /** POT ************************************************************/

    #define mInitPOT() 


//Uncomment below if using the YOUR_BOARD hardware platform
//#elif defined(YOUR_BOARD)
//Add your hardware specific I/O pin mapping here

#else
    #error Not a supported board (yet), add I/O pin mapping in __FILE__, line __LINE__
#endif

#endif //IO_CFG_H
