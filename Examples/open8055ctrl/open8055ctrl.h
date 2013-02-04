#ifndef _OPEN8055CTRL_H
#define _OPEN8055CTRL_H

extern int	terminateFlag;
extern int	commandTag;
extern OPEN8055_HANDLE	cardHandle;

extern int	yyparse(void);
extern void	yyerror(const char *msg);
extern void	scan_new_command(char *cmd);

#endif
