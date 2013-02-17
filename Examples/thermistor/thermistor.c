/* ----------------------------------------------------------------------
 * thermistor.c
 *
 *	Example program demonstrating how to measure temperature with
 *	a thermistor.
 * ----------------------------------------------------------------------
 */


#include <stdio.h>

#include "open8055.h"

/* ----
 * The following values are from the thermistor datasheet.
 *
 *	Beta in this case is the R/T characteristic of a
 *	B57560G 10K NTC thermistor.
 *	R1 is the nominal resistance at T1 degrees Celsius.
 * ----
 */
#define	Beta		3390
#define R1			10000
#define	T1			25


int
main(int argc, char *argv[])
{
	char	*destination = "card0";
	int		card;

	/* ----
	 * The default is "card0". A different Open8055 card can
	 * be specified on the command line.
	 * ----
	 */
	if (argc > 1)
		destination = argv[1];

	/* ----
	 * Try to connect to the Open8055.
	 * ----
	 */
	card = Open8055_Connect(destination, NULL);
	if (card < 0)
	{
		fprintf(stderr, "%s: %s\n", destination, Open8055_LastError(-1));
		return 2;
	}

	/* ----
	 * 8 bit ADC resolution is all we need for this example.
	 * ----
	 */
	Open8055_SetModeADC(card, 0, OPEN8055_MODE_ADC8);

	/* ----
	 * Loop until the user aborts the program with CTRL-C.
	 * ----
	 */
	for(;;)
	{
		double	R2;
		double	ADCvalue;
		double	T1kelvin;
		double	T2kelvin;

		/* ----
		 * Measure the current resistance of the thermistor.
		 * ----
		 */
		ADCvalue = (double)Open8055_GetADC(card, 0) / 256.0;
		R2 = 1.0 / ((ADCvalue / 23 * 11) / 3900.0) - 3900.0;

		/* ----
		 * Knowing that we can calculate the temperature.
		 * ----
		 */
		T1kelvin = T1 + 273.15;
		T2kelvin = T1kelvin * Beta / log(R1/R2) / 
					(Beta / log(R1/R2) - T1kelvin);

		printf("\r%5.1f C %5.1f F", T2kelvin - 273.15,
					(T2kelvin - 273.15) / 5.0 * 9.0 + 32.0);
		fflush(stdout);

		usleep(1000000);
		if (Open8055_Wait(card) < 0)
		{
			printf("\r                   \r");
			fflush(stdout);
			fprintf(stderr, "%s: %s\n", destination, Open8055_LastError(card));
			break;
		}
	}

	fprintf(stderr, "%s: %s\n", destination, Open8055_LastError(card));
	Open8055_Close(card);

	return 0;
}


