/********************************************************************
 FileName:     	HardwareProfile_PIC18F2550.h
 Dependencies:  See INCLUDES section
 Processor:     PIC18F2550 USB Microcontroller
 Hardware:      Open8055 based on Vellaman K8055(N)
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

#ifndef HARDWARE_PROFILE_PIC18F2550_H
#define HARDWARE_PROFILE_PIC18F2550_H

    //ADCON0
    // bit7..6 	- Unimplemented
    // bit5..2 	- CHS3:0, channel select
    // bit1	- GO/DONE
    // bit1	- ADON
    #define OPEN8055_ADCON0 0x01


    // ADCON1
    // bit7..6 	- Unimplemented
    // bit5..4	- VCFG1..0, 00 = supply rails
    // bit3..0	- PCGG3..0, 1101 = AIN1..0 only
    #define OPEN8055_ADCON1 0x0D


    // ADCON2
    // bit 7
    //		1 = Right justified
    //		0 = Left justified
    // bit 5-3 ACQT2:ACQT0: A/D Acquisition Time Select bits
    //		111 = 20 TAD
    //		110 = 16 TAD
    //		101 = 12 TAD
    //		100 = 8 TAD
    //		011 = 6 TAD
    //		010 = 4 TAD
    //		001 = 2 TAD
    //		000 = 0 TAD(1)
    // bit 2-0 ADCS2:ADCS0: A/D Conversion Clock Select bits
    //		111 = FRC (clock derived from A/D RC oscillator)(1)
    //		110 = FOSC/64
    //		101 = FOSC/16
    //		100 = FOSC/4
    //		011 = FRC (clock derived from A/D RC oscillator)(1)
    //		010 = FOSC/32
    //		001 = FOSC/8
    //		000 = FOSC/2
    #define OPEN8055_ADCON2 0b10111111 // Right,20TACQ,FRC



    
#endif  //HARDWARE_PROFILE_PIC18F2550_H
