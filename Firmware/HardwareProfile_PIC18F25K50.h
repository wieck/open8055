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

    /** IO PORT SETTINGS ***********************************************/

    //ADCON1
    //	Setting these bits in ADCON1 configures all ports to digital.
    #define OPEN8055_ADCON1_ALL_DIGITAL_MASK	0x0F
    
#endif  //HARDWARE_PROFILE_PIC18F2550_H
