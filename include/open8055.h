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
 *	Copyright (c) 2012, Jan Wieck
 *	All rights reserved.
 *	
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions are met:
 *		* Redistributions of source code must retain the above copyright
 *		  notice, this list of conditions and the following disclaimer.
 *		* Redistributions in binary form must reproduce the above copyright
 *		  notice, this list of conditions and the following disclaimer in the
 *		  documentation and/or other materials provided with the distribution.
 *		* Neither the name of the <organization> nor the
 *		  names of its contributors may be used to endorse or promote products
 *		  derived from this software without specific prior written permission.
 *	
 *	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *	DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 *	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *	
 * ----------------------------------------------------------------------
 */

#ifndef _OPEN8055_H
#define _OPEN8055_H

#include "open8055_compat.h"
#include "open8055_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

#define OPEN8055_EXTERN		_CRTIMP
#define OPEN8055_CDECL		__cdecl

#else /* !_WIN32 */

#define OPEN8055_EXTERN extern
#define OPEN8055_CDECL

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

#define OPEN8055_MAX_CARDS			16
#define OPEN8055_WAITFOR_MS			1
#define OPEN8055_INFINITE			-1


/* ----
 * The following bits define unique input items in the reports.
 * These can be used as a bitmask when waiting for status changes.
 * ----
 */
#define OPEN8055_INPUT_I1			0x0001
#define OPEN8055_INPUT_I2			0x0002
#define OPEN8055_INPUT_I3			0x0004
#define OPEN8055_INPUT_I4			0x0008
#define OPEN8055_INPUT_I5			0x0010
#define OPEN8055_INPUT_I_ANY		0x001F

#define OPEN8055_INPUT_COUNT1		0x0020
#define OPEN8055_INPUT_COUNT2		0x0040
#define OPEN8055_INPUT_COUNT3		0x0080
#define OPEN8055_INPUT_COUNT4		0x0100
#define OPEN8055_INPUT_COUNT5		0x0200
#define OPEN8055_INPUT_COUNT_ANY	0x03E0

#define OPEN8055_INPUT_ADC1			0x0400
#define OPEN8055_INPUT_ADC2			0x0800
#define OPEN8055_INPUT_ADC_ANY		0x0C00

#define OPEN8055_INPUT_ANY			0x0FFF

/* ----
 * Declarations
 * ----
 */


/* ----
 * Public functions in open8055.c
 * ----
 */
OPEN8055_EXTERN char	*OPEN8055_CDECL Open8055_LastError(int h);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_CardPresent(int cardNumber);

OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_Connect(char *destination, char *password);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_Close(int h);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_Reset(int h);

OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_Wait(int h);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_WaitTimeout(int h, int timeout);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_WaitEx(int h, int timeout, int skipMessages);
OPEN8055_EXTERN void	OPEN8055_CDECL Open8055_Sleep(int ms);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_GetAutoFlush(int h);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_SetAutoFlush(int h, int flag);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_Flush(int h);

OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_GetInput(int h, int port);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_GetInputAll(int h);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_GetCounter(int h, int port);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_ResetCounter(int h, int port);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_ResetCounterAll(int h);
OPEN8055_EXTERN double	OPEN8055_CDECL Open8055_GetDebounce(int h, int port);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_SetDebounce(int h, int port, double value);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_GetADC(int h, int port);

OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_GetOutput(int h, int port);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_GetOutputAll(int h);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_GetOutputValue(int h, int port);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_GetPWM(int h, int port);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_SetOutput(int h, int port, int val);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_SetOutputAll(int h, int val);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_SetOutputValue(int h, int port, int val);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_SetPWM(int h, int port, int val);

OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_GetModeADC(int h, int port);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_SetModeADC(int h, int port, int mode);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_GetModeInput(int h, int port);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_SetModeInput(int h, int port, int mode);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_GetModeOutput(int h, int port);
OPEN8055_EXTERN int		OPEN8055_CDECL Open8055_SetModeOutput(int h, int port, int mode);


#ifdef __cplusplus
}
#endif

#endif /* _OPEN8055_H */

