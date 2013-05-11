%{
/* ----------------------------------------------------------------------
 * gram.y
 *
 *	The bison parser for the open8055ctrl utility.
 * ----------------------------------------------------------------------
 *
 *	Copyright (c) 2013, Jan Wieck
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
#include "open8055_compat.h"
#include "open8055.h"
#include "open8055ctrl.h"

#include "scan.c"

/* ----
 * Local variables.
 * ----
 */
static int		intRc;
static double	doubleRc;

/* ----
 * Local functions.
 * ----
 */
static int		parseInteger(char *str);
static double	parseDouble(char *str);
static void		displayCardError(void);

%}

/* ----
 * YYSTYPE
 * ----
 */
%union {
	int		ival;
	double	dval;
}

%token		T_UNKNOWN_CMD
%token		T_INTEGER
%token		T_DOUBLE

%token		C_LastError
%token		C_CardPresent

%token		C_Close
%token		C_Reset

%token		C_Wait
%token		C_WaitTimeout
%token		C_WaitEx
%token		C_GetAutoFlush
%token		C_SetAutoFlush
%token		C_Flush

%token		C_GetInput
%token		C_GetInputAll
%token		C_GetADC
%token		C_GetCounter
%token		C_ResetCounter
%token		C_ResetCounterAll
%token		C_GetDebounce
%token		C_SetDebounce

%token		C_GetOutput
%token		C_GetOutputAll
%token		C_GetOutputValue
%token		C_GetPWM
%token		C_SetOutput
%token		C_SetOutputAll
%token		C_SetOutputValue
%token		C_SetPWM

%token		C_GetModeADC
%token		C_SetModeADC
%token		C_GetModeInput
%token		C_SetModeInput


%type <ival>	ival
%type <dval>	dval


%%

command					:
						| cmd_LastError
						| cmd_CardPresent
						| cmd_Close
						| cmd_Reset
						| cmd_Wait
						| cmd_WaitTimeout
						| cmd_WaitEx
						| cmd_GetAutoFlush
						| cmd_SetAutoFlush
						| cmd_Flush
						| cmd_GetInput
						| cmd_GetInputAll
						| cmd_GetADC
						| cmd_GetCounter
						| cmd_ResetCounter
						| cmd_ResetCounterAll
						| cmd_GetDebounce
						| cmd_SetDebounce
						| cmd_GetOutput
						| cmd_GetOutputAll
						| cmd_GetPWM
						| cmd_SetOutput
						| cmd_SetOutputAll
						| cmd_SetPWM
						| cmd_GetModeADC
						| cmd_SetModeADC
						| cmd_GetModeInput
						| cmd_SetModeInput

cmd_LastError			: C_LastError
						{
							displayCardError();
						}

cmd_CardPresent			: C_CardPresent ival
						{
							if ((intRc = Open8055_CardPresent($2)) < 0)
								displayCardError();
							else
							printf("%d\n", intRc);
						}

cmd_Close				: C_Close
						{
							terminateFlag = TRUE;
						}

cmd_Reset				: C_Reset
						{
							if ((intRc = Open8055_Reset(cardHandle)) < 0)
								displayCardError();
							exit(0);
						}

cmd_Wait				: C_Wait
						{
							if ((intRc = Open8055_Wait(cardHandle)) < 0)
								displayCardError();
							else
								printf("%d\n", intRc);
						}

cmd_WaitTimeout			: C_WaitTimeout ival
						{
							if ((intRc = Open8055_WaitTimeout(cardHandle, $2)) < 0)
								displayCardError();
							else
								printf("%d\n", intRc);
						}

cmd_WaitEx				: C_WaitEx ival ival
						{
							if ((intRc = Open8055_WaitEx(cardHandle, $2, $3)) < 0)
								displayCardError();
							else
								printf("%d\n", intRc);
						}

cmd_GetAutoFlush		: C_GetAutoFlush
						{
							if ((intRc = Open8055_GetAutoFlush(cardHandle)) < 0)
								displayCardError();
							else
								printf("%d\n", intRc);
						}

cmd_SetAutoFlush		: C_SetAutoFlush ival
						{
							if ((intRc = Open8055_SetAutoFlush(cardHandle, $2)) < 0)
								displayCardError();
						}

cmd_Flush				: C_Flush
						{
							if ((intRc = Open8055_Flush(cardHandle)) < 0)
								displayCardError();
						}

cmd_GetInput		: C_GetInput ival
						{
							if ((intRc = Open8055_GetInput(cardHandle, $2)) < 0)
								displayCardError();
							else
								printf("%d\n", intRc);
						}

cmd_GetInputAll : C_GetInputAll
						{
							if ((intRc = Open8055_GetInputAll(cardHandle)) < 0)
								displayCardError();
							else
								printf("%d\n", intRc);
						}

cmd_GetADC			: C_GetADC ival
						{
							if ((intRc = Open8055_GetADC(cardHandle, $2)) < 0)
								displayCardError();
							else
								printf("%d\n", intRc);
						}

cmd_GetCounter		: C_GetCounter ival
						{
							if ((intRc = Open8055_GetCounter(cardHandle, $2)) < 0)
								displayCardError();
							else
								printf("%d\n", intRc);
						}

cmd_ResetCounter	: C_ResetCounter ival
						{
							if ((intRc = Open8055_ResetCounter(cardHandle, $2)) < 0)
								displayCardError();
						}

cmd_ResetCounterAll: C_ResetCounterAll
				{
					if ((intRc = Open8055_ResetCounterAll(cardHandle)) < 0)
						displayCardError();
				}

cmd_GetDebounce		: C_GetDebounce ival
				{
					doubleRc = Open8055_GetDebounce(cardHandle, $2);
					printf("%.1f\n", doubleRc);
				}

cmd_SetDebounce		: C_SetDebounce ival dval
				{
					if ((intRc = Open8055_SetDebounce(cardHandle, $2, $3)) < 0)
						displayCardError();
				}

cmd_GetOutput		: C_GetOutput ival
				{
					if ((intRc = Open8055_GetOutput(cardHandle, $2)) < 0)
						displayCardError();
					else
						printf("%d\n", intRc);
				}

cmd_GetOutputAll		: C_GetOutputAll
				{
					if ((intRc = Open8055_GetOutputAll(cardHandle)) < 0)
						displayCardError();
					else
						printf("%d\n", intRc);
				}

cmd_GetPWM		: C_GetPWM ival
				{
					if ((intRc = Open8055_GetPWM(cardHandle, $2)) < 0)
						displayCardError();
					else
						printf("%d\n", intRc);
				}

cmd_SetOutput		: C_SetOutput ival ival
				{
					if ((intRc = Open8055_SetOutput(cardHandle, $2, $3)) < 0)
						displayCardError();
				}

cmd_SetOutputAll	: C_SetOutputAll ival
				{
					if ((intRc = Open8055_SetOutputAll(cardHandle, $2)) < 0)
						displayCardError();
				}

cmd_SetPWM		: C_SetPWM ival ival
				{
					if ((intRc = Open8055_SetPWM(cardHandle, $2, $3)) < 0)
						displayCardError();
				}

cmd_GetModeADC	: C_GetModeADC ival
				{
					if ((intRc = Open8055_GetModeADC(cardHandle, $2)) < 0)
						displayCardError();
					else
					    printf("%d\n", intRc);
				}

cmd_SetModeADC	: C_SetModeADC ival ival
				{
					if ((intRc = Open8055_SetModeADC(cardHandle, $2, $3)) < 0)
						displayCardError();
				}

cmd_GetModeInput	: C_GetModeInput ival
				{
					if ((intRc = Open8055_GetModeInput(cardHandle, $2)) < 0)
						displayCardError();
					else
					    printf("%d\n", intRc);
				}

cmd_SetModeInput	: C_SetModeInput ival ival
				{
					if ((intRc = Open8055_SetModeInput(cardHandle, $2, $3)) < 0)
						displayCardError();
				}

ival				: T_INTEGER
				{
					$$ = parseInteger(yytext);
				}

dval				: T_DOUBLE
				{
					$$ = parseDouble(yytext);
				}
				| T_INTEGER
				{
					$$ = (double)parseInteger(yytext);
				}
%%

static int
parseInteger(char *str)
{
	int result;

	sscanf(str, "%i", &result);
	return result;
}

static double
parseDouble(char *str)
{
	double result;

	sscanf(str, "%lf", &result);
	return result;
}

static void
displayCardError(void)
{
	fprintf(stderr, "Error: %s\n", Open8055_LastError(cardHandle));
}

void
yyerror(const char *msg)
{
	fprintf(stderr, "Error: %s at or near %s\n", msg, yytext);
}
