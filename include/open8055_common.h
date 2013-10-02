/* ----------------------------------------------------------------------
 * open8055.h
 *
 *  Definitions that are needed in open8055.h and open8055_hid_protocol.h.
 *
 * ----------------------------------------------------------------------
 *
 *  Copyright (c) 2012, Jan Wieck
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *      * Neither the name of the <organization> nor the
 *        names of its contributors may be used to endorse or promote products
 *        derived from this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *  
 * ----------------------------------------------------------------------
 */

#ifndef _OPEN8055_COMMON_H
#define _OPEN8055_COMMON_H


#define OPEN8055_MODE_ADC10         10  // A1,A2 - port is in ADC mode 10 bits resolution
#define OPEN8055_MODE_ADC9          11  // A1,A2 - port is in ADC mode masked to 9 bits
#define OPEN8055_MODE_ADC8          12  // A1,A2 - port is in ADC mode masked to 8 bits
#define OPEN8055_MODE_INPUT         20  // I1..I5 - port is digital input
#define OPEN8055_MODE_FREQUENCY     21  // I1..I5 - port is a frequency counter
#define OPEN8055_MODE_EUSART        22  // I4&I5 - ports used as EUSART
#define OPEN8055_MODE_OUTPUT        30  // O1..O8 - port is digital output
#define OPEN8055_MODE_SERVO         31  // O1..O8 - port is in servo mode
#define OPEN8055_MODE_ISERVO        32  // O1..O8 - port is in inverted servo mode
#define OPEN8055_MODE_I2C           33  // O1&O2 - ports used as I2C bus.
#define OPEN8055_MODE_PWM           40  // PWM1,PWM2 - port used as PWM output


#endif

