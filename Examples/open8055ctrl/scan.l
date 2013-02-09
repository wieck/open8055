%{
/* ----------------------------------------------------------------------
 * scan.l
 *
 *	The flex scanner of the open8055ctrl utility.
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

LastError				{ return command(C_LastError);			}
CardPresent				{ return command(C_CardPresent);		}

Close					{ return command(C_Close);				}
Quit					{ return command(C_Close);				}
Reset					{ return command(C_Reset);				}

Wait					{ return command(C_Wait);				}
WaitTimeout				{ return command(C_WaitTimeout);		}
WaitEx					{ return command(C_WaitEx);				}
GetAutoFlush			{ return command(C_GetAutoFlush);		}
SetAutoFlush			{ return command(C_SetAutoFlush);		}
Flush					{ return command(C_Flush);				}

GetInput				{ return command(C_GetInput);			}
GetInputAll				{ return command(C_GetInputAll);		}
GetADC					{ return command(C_GetADC);				}
GetCounter				{ return command(C_GetCounter);			}
ResetCounter			{ return command(C_ResetCounter);		}
ResetCounterAll			{ return command(C_ResetCounterAll);	}
GetDebounce				{ return command(C_GetDebounce);		}
SetDebounce				{ return command(C_SetDebounce);		}

GetOutput				{ return command(C_GetOutput);			}
GetOutputAll			{ return command(C_GetOutputAll);		}
GetPWM					{ return command(C_GetPWM);				}
SetOutput				{ return command(C_SetOutput);			}
SetOutputAll			{ return command(C_SetOutputAll);		}
SetPWM					{ return command(C_SetPWM);				}

GetModeInput			{ return command(C_GetModeInput);		}
SetModeInput			{ return command(C_SetModeInput);		}

0x{hexdigit}+			{ return T_INTEGER;						}
{digit}+				{ return T_INTEGER;						}
{digit}+\.{digit}*		{ return T_DOUBLE;						}
\.{digit}+				{ return T_DOUBLE;						}

{space}					{}

.						{ return yytext[0];						}


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


