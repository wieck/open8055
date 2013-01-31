/* ----------------------------------------------------------------------
 * open8055.h
 *
 *	open8055 implements the HID protocol communication with an
 *	Open8055 USB Experiment Interface Board. Under Unix type systems,
 *	this module expects libusb 1.0. Under Windows, native calls
 *	to the setupapi.dll are used instead.
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

#ifndef _OPEN8055_H
#define _OPEN8055_H


#ifdef __cplusplus
"C" {
#endif

#ifdef _WIN32

#ifdef _WIN64
#  define STDCALL __stdcall
#endif

#ifdef OPEN8055_BUILD_DLL
#  define OPEN8055_EXTERN __declspec(dllexport)
#else
#  ifdef OPEN8055_STATIC
#    define OPEN8055_EXTERN extern
#  else
#    define OPEN8055_EXTERN __declspec(dllimport)
#  endif
#endif /* BUILD_DLL */

#else /* !_WIN32 */

#define STDCALL
#define OPEN8055_EXTERN extern

#endif /* _WIN32 */


/* ----
 * Identifiers needed to find the USB device.
 * ----
 */
#define OPEN8055_GUID	"4d1e55b2-f16f-11cf-88cb-001111000030"
#define OPEN8055_VID	0x10cf
#define OPEN8055_PID	0x55f0


/* ----
 * Definitions
 * ----
 */

#define OPEN8055_MAX_CARDS	3
#define OPEN8055_WAITFOR_US	1000

/* ----
 * The following bits define unique input items in the reports.
 * These can be used as a bitmask when waiting for status changes.
 * ----
 */
#define	OPEN8055_INPUT_I1		0b00000000000000000000000000000001
#define	OPEN8055_INPUT_I2		0b00000000000000000000000000000010
#define	OPEN8055_INPUT_I3		0b00000000000000000000000000000100
#define	OPEN8055_INPUT_I4		0b00000000000000000000000000001000
#define	OPEN8055_INPUT_I5		0b00000000000000000000000000010000
#define	OPEN8055_INPUT_I_ANY		0b00000000000000000000000000011111

#define	OPEN8055_INPUT_COUNT1		0b00000000000000000000000000100000
#define	OPEN8055_INPUT_COUNT2		0b00000000000000000000000001000000
#define	OPEN8055_INPUT_COUNT3		0b00000000000000000000000010000000
#define	OPEN8055_INPUT_COUNT4		0b00000000000000000000000100000000
#define	OPEN8055_INPUT_COUNT5		0b00000000000000000000001000000000
#define	OPEN8055_INPUT_COUNT_ANY	0b00000000000000000000001111100000

#define	OPEN8055_INPUT_ADC1		0b00000000000000000000010000000000
#define	OPEN8055_INPUT_ADC2		0b00000000000000000000100000000000
#define	OPEN8055_INPUT_ADC_ANY		0b00000000000000000000110000000000

#define	OPEN8055_INPUT_ANY		0b00000000000000000000111111111111

/* ----
 * Declarations
 * ----
 */


/* ----
 * Public functions in open8055.c
 * ----
 */
OPEN8055_EXTERN char *		STDCALL Open8055_LastError(void);
OPEN8055_EXTERN char *		STDCALL Open8055_LastErrorCopy(void);
OPEN8055_EXTERN int		STDCALL Open8055_CardPresent(int cardNumber);

OPEN8055_EXTERN int		STDCALL Open8055_Connect(char *destination, char *password);
OPEN8055_EXTERN int		STDCALL Open8055_Close(int h);

OPEN8055_EXTERN int		STDCALL Open8055_Wait(int h, long us);
OPEN8055_EXTERN int		STDCALL Open8055_WaitFor(int h, uint32_t mask, long us);
OPEN8055_EXTERN int		STDCALL Open8055_OutputBuffering(int h, int flag);
OPEN8055_EXTERN int		STDCALL Open8055_OutputFlush(int h);

OPEN8055_EXTERN int		STDCALL Open8055_GetInputDigital(int h, int port);
OPEN8055_EXTERN int		STDCALL Open8055_GetInputDigitalAll(int h);
OPEN8055_EXTERN int		STDCALL Open8055_GetInputADC(int h, int port);
OPEN8055_EXTERN int		STDCALL Open8055_GetInputCounter(int h, int port);
OPEN8055_EXTERN int		STDCALL Open8055_ResetInputCounter(int h, int port);
OPEN8055_EXTERN int		STDCALL Open8055_ResetInputCounters(int h);

OPEN8055_EXTERN int		STDCALL Open8055_GetOutputDigital(int h, int port);
OPEN8055_EXTERN int		STDCALL Open8055_GetOutputDigitalAll(int h);
OPEN8055_EXTERN int		STDCALL Open8055_GetOutputValue(int h, int port);
OPEN8055_EXTERN int		STDCALL Open8055_GetOutputPWM(int h, int port);
OPEN8055_EXTERN int		STDCALL Open8055_SetOutputDigital(int h, int port, int val);
OPEN8055_EXTERN int		STDCALL Open8055_SetOutputDigitalAll(int h, int val);
OPEN8055_EXTERN int		STDCALL Open8055_SetOutputValue(int h, int port, int val);
OPEN8055_EXTERN int		STDCALL Open8055_SetOutputPWM(int h, int port, int val);


#ifdef __cplusplus
}
#endif

#endif /* _OPEN8055_H */

