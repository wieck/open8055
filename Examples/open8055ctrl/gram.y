%{
#include "open8055_compat.h"
#include "open8055.h"
#include "open8055ctrl.h"

#include "scan.h"

static int	intRc;
static double	doubleRc;
static int	parseInteger(char *str);
static double	parseDouble(char *str);
static void	displayCardError(void);
%}

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
%token		C_WaitFor
%token		C_GetAutoFlush
%token		C_SetAutoFlush
%token		C_Flush

%token		C_GetInputDigital
%token		C_GetInputDigitalAll
%token		C_GetInputADC
%token		C_GetInputCounter
%token		C_ResetInputCounter
%token		C_ResetInputCounterAll
%token		C_GetInputDebounce
%token		C_SetInputDebounce

%token		C_GetOutputDigital
%token		C_GetOutputDigitalAll
%token		C_GetOutputValue
%token		C_GetOutputPWM
%token		C_SetOutputDigital
%token		C_SetOutputDigitalAll
%token		C_SetOutputValue
%token		C_SetOutputPWM


%type <ival>	ival
%type <dval>	dval


%%

command				:
				| cmd_LastError
				| cmd_CardPresent
				| cmd_Close
				| cmd_Reset
				| cmd_Wait
				| cmd_WaitFor
				| cmd_GetAutoFlush
				| cmd_SetAutoFlush
				| cmd_Flush
				| cmd_GetInputDigital
				| cmd_GetInputDigitalAll
				| cmd_GetInputADC
				| cmd_GetInputCounter
				| cmd_ResetInputCounter
				| cmd_ResetInputCounterAll
				| cmd_GetInputDebounce
				| cmd_SetInputDebounce
				| cmd_GetOutputDigital
				| cmd_GetOutputDigitalAll
				| cmd_GetOutputPWM
				| cmd_SetOutputDigital
				| cmd_SetOutputDigitalAll
				| cmd_SetOutputPWM

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

cmd_Close			: C_Close
				{
				    terminateFlag = TRUE;
				}

cmd_Reset			: C_Reset
				{
				    if ((intRc = Open8055_Reset(cardHandle)) < 0)
				    	displayCardError();
				    exit(0);
				}

cmd_Wait			: C_Wait ival
				{
				    if ((intRc = Open8055_Wait(cardHandle, $2)) < 0)
				    	displayCardError();
				}

cmd_WaitFor			: C_WaitFor ival ival
				{
				    if ((intRc = Open8055_WaitFor(cardHandle, $2, $3)) < 0)
				    	displayCardError();
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

cmd_Flush			: C_Flush
				{
				    if ((intRc = Open8055_Flush(cardHandle)) < 0)
				    	displayCardError();
				}

cmd_GetInputDigital		: C_GetInputDigital ival
				{
				    if ((intRc = Open8055_GetInputDigital(cardHandle, $2)) < 0)
				    	displayCardError();
				    else
					printf("%d\n", intRc);
				}

cmd_GetInputDigitalAll		: C_GetInputDigitalAll
				{
				    if ((intRc = Open8055_GetInputDigitalAll(cardHandle)) < 0)
				    	displayCardError();
				    else
					printf("%d\n", intRc);
				}

cmd_GetInputADC			: C_GetInputADC ival
				{
				    if ((intRc = Open8055_GetInputADC(cardHandle, $2)) < 0)
				    	displayCardError();
				    else
					printf("%d\n", intRc);
				}

cmd_GetInputCounter		: C_GetInputCounter ival
				{
				    if ((intRc = Open8055_GetInputCounter(cardHandle, $2)) < 0)
				    	displayCardError();
				    else
					printf("%d\n", intRc);
				}

cmd_ResetInputCounter		: C_ResetInputCounter ival
				{
				    if ((intRc = Open8055_ResetInputCounter(cardHandle, $2)) < 0)
				    	displayCardError();
				}

cmd_ResetInputCounterAll	: C_ResetInputCounterAll
				{
				    if ((intRc = Open8055_ResetInputCounterAll(cardHandle)) < 0)
				    	displayCardError();
				}

cmd_GetInputDebounce		: C_GetInputDebounce ival
				{
				    doubleRc = Open8055_GetInputDebounce(cardHandle, $2);
				    printf("%.1f\n", doubleRc);
				}

cmd_SetInputDebounce		: C_SetInputDebounce ival dval
				{
				    if ((intRc = Open8055_SetInputDebounce(cardHandle, $2, $3)) < 0)
				    	displayCardError();
				}

cmd_GetOutputDigital		: C_GetOutputDigital ival
				{
				    if ((intRc = Open8055_GetOutputDigital(cardHandle, $2)) < 0)
				    	displayCardError();
				    else
				    	printf("%d\n", intRc);
				}

cmd_GetOutputDigitalAll		: C_GetOutputDigitalAll
				{
				    if ((intRc = Open8055_GetOutputDigitalAll(cardHandle)) < 0)
				    	displayCardError();
				    else
				    	printf("%d\n", intRc);
				}

cmd_GetOutputPWM		: C_GetOutputPWM ival
				{
				    if ((intRc = Open8055_GetOutputPWM(cardHandle, $2)) < 0)
				    	displayCardError();
				    else
				    	printf("%d\n", intRc);
				}

cmd_SetOutputDigital		: C_SetOutputDigital ival ival
				{
				    if ((intRc = Open8055_SetOutputDigital(cardHandle, $2, $3)) < 0)
				        displayCardError();
				}

cmd_SetOutputDigitalAll		: C_SetOutputDigitalAll ival
				{
				    if ((intRc = Open8055_SetOutputDigitalAll(cardHandle, $2)) < 0)
				        displayCardError();
				}

cmd_SetOutputPWM		: C_SetOutputPWM ival ival
				{
				    if ((intRc = Open8055_SetOutputPWM(cardHandle, $2, $3)) < 0)
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
    int	result;

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
