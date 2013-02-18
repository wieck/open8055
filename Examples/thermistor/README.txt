README for Open8055 thermistor project.


This example project shows how to connect a 10K Ohm Negative Temperature
Coefficient resistor (NTC) to an Open8055 board and use it.

The schematics, PCB design and printable PCB images can be found in the
circuit subdirectory. You will need ExpressSCH and ExpressPCB to open and
edit the schematic and PCB source files. The software is available for 
free from

    http://www.expresspcb.com/expresspcbhtm/Free_schematic_software.htm

This example is using a B57560G NTC. This thermistor has a nominal resistance
of 10K Ohm at 25 degrees Celsius and a Beta value of 3390 in the temperature
range from 0 to 100 degrees Celsius.

The intended range is 0 to 50 degrees Celsius. The example circuit uses a
dual OP-AMP (MCP6002, but most other OP-AMPs will work) to accomplish two
things:

    * Stabilize a 1/11th voltage divider for intput into the thermistor
	  based voltage divider. This is done to minimize the power, dissipated
	  by the thermistor itself. After all, a resistor dissipating power is
	  a heating element by itself. That isn't desired when using it for
	  measuring temperature.

	* Amplify the thermistor voltage divider's output so that the result
	  just fits into the 0 to +5V input range of the Open8055 ADC. This makes
	  maximum use of the available resolution.


The example program thermistor.c is a command line utility that simply prints
the current temperature reading every 500ms in Celsius and Fahrenheit. The
math behind it is explained in the comments.
