/*
 *
 */


#include <stdio.h>

#include "open8055.h"


int
main(int argc, char *argv[])
{
	char	*destination = "card0";
	int		card;

	if (argc > 1)
		destination = argv[1];

	card = Open8055_Connect(destination, NULL);
	if (card < 0)
	{
		fprintf(stderr, "%s: %s\n", destination, Open8055_LastError(-1));
		return 2;
	}

	Open8055_SetModeADC(card, 0, OPEN8055_MODE_ADC8);

	for(;;)
	{
		printf("ADC1=%d     \r", Open8055_GetADC(card, 0));
		fflush(stdout);

		if (Open8055_Wait(card) < 0)
			break;
	}

	fprintf(stderr, "%s: %s\n", destination, Open8055_LastError(card));
	Open8055_Close(card);

	return 0;
}
