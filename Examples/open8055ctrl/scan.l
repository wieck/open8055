%{
#include "open8055_compat.h"
#include "open8055.h"
#include "open8055ctrl.h"
#include "gram.h"

static int command(int tag);

%}

%option noyywrap
%option nounput
%option case-insensitive

space		[ \t\n\r\f]
digit		[0-9]
hexdigit	[0-9a-f]

%%

LastError			{ return command(C_LastError);			}
CardPresent			{ return command(C_CardPresent);		}
GetSkipMessages			{ return command(C_GetSkipMessages);		}
SetSkipMessages			{ return command(C_SetSkipMessages);		}

Close				{ return command(C_Close);			}
Quit				{ return command(C_Close);			}
Reset				{ return command(C_Reset);			}

Wait				{ return command(C_Wait);			}
WaitFor				{ return command(C_WaitFor);			}
GetAutoFlush			{ return command(C_GetAutoFlush);		}
SetAutoFlush			{ return command(C_SetAutoFlush);		}
Flush				{ return command(C_Flush);			}

GetInputDigital			{ return command(C_GetInputDigital);		}
GetInputDigitalAll		{ return command(C_GetInputDigitalAll);		}
GetInputADC			{ return command(C_GetInputADC);		}
GetInputCounter			{ return command(C_GetInputCounter);		}
ResetInputCounter		{ return command(C_ResetInputCounter);		}
ResetInputCounterAll		{ return command(C_ResetInputCounterAll);	}
GetInputDebounce		{ return command(C_GetInputDebounce);		}
SetInputDebounce		{ return command(C_SetInputDebounce);		}

GetOutputDigital		{ return command(C_GetOutputDigital);		}
GetOutputDigitalAll		{ return command(C_GetOutputDigitalAll);	}
GetOutputPWM			{ return command(C_GetOutputPWM);		}
SetOutputDigital		{ return command(C_SetOutputDigital);		}
SetOutputDigitalAll		{ return command(C_SetOutputDigitalAll);	}
SetOutputPWM			{ return command(C_SetOutputPWM);		}

0x{hexdigit}+			{ return T_INTEGER;				}
{digit}+			{ return T_INTEGER;				}
{digit}+\.{digit}*		{ return T_DOUBLE;				}
\.{digit}+			{ return T_DOUBLE;				}

{space}				{}

.				{ return yytext[0];				}


%%

void
scan_new_command(char *cmd)
{
    if (YY_CURRENT_BUFFER)
        yy_delete_buffer(YY_CURRENT_BUFFER);

    yy_switch_to_buffer(yy_scan_string(cmd));
}

static int
command(int tag)
{
    if (commandTag == T_UNKNOWN_CMD)
        commandTag = tag;
    return tag;
}


