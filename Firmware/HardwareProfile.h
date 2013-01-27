/********************************************************************
 FileName:      HardwareProfile.h
 Dependencies:  See INCLUDES section
 Processor:     PIC18, PIC24, or PIC32 USB Microcontrollers
 Hardware:      The code is natively intended to be used on the 
                  following hardware platforms: 
                    PICDEM™ FS USB Demo Board
                    PIC18F46J50 FS USB Plug-In Module
                    PIC18F87J50 FS USB Plug-In Module
                    Explorer 16 + PIC24 or PIC32 USB PIMs
                    PIC24F Starter Kit
                    Low Pin Count USB Development Kit
                  The firmware may be modified for use on other USB 
                    platforms by editing this file (HardwareProfile.h)
 Compiler:  	Microchip C18 (for PIC18), C30 (for PIC24), 
                  or C32 (for PIC32)
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
  
  0.1	11/13/2012	Adjusted for Open8055 Project
********************************************************************/

#ifndef HARDWARE_PROFILE_H
#define HARDWARE_PROFILE_H


/* ----
 * Processor and Board independent settings.
 * ----
 */

// Open8055 firmware is always running on 12 MHz. That means that
// the actual clock frequency is 48 MHz.
#define CLOCK_FREQ 48000000

// Open8055 firmware is HID Bootloader friendly.
#define PROGRAMMABLE_WITH_USB_HID_BOOTLOADER		

// Timer3 is used for a 100us ticker. At 12 MHz 100 microseconds
// would be 1200 cycles. The missing 40 cycles elapse while the
// timer is turned off and reloaded.
#define OPEN8055_TICK_TIMER_CYCLES		1160
#define OPEN8055_TICKS_PER_MS			10


// IO pin definitions
#define INPUT_PIN 1
#define OUTPUT_PIN 0


/* ----
 * Include the Processor specific Hardware Profile
 * ----
 */
#if defined(__18F2550)
    #include "HardwareProfile_PIC18F2550.h"
#elif defined(__18F25K50)
    #include "HardwareProfile_PIC18F25K50.h"
#else
    #error "Unsupported Processor for Open8055 Project file __FILE__, line __LINE__"
#endif


/* ----
 * Include the Board specific Hardware Profile
 * 
 * BOARD1 is the K8055-1
 * BOARD2 is the K8055N-2
 * ----
 */
#if (OPEN8055_PCB == P8055-1)
    #include "HardwareProfile_P8055-1.h"
#elif (OPEN8055_PCB == P8055N-2)
    #include "HardwareProfile_P8055N-2.h"
#else
    #error "Unsupported Board Type for Open8055 Project file __FILE__, line __LINE__"
#endif


#endif  //HARDWARE_PROFILE_H
