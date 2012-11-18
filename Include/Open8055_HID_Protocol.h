#ifndef OPEN8055_HID_PROTOCOL_H
#define OPEN8055_HID_PROTOCOL_H


// We need a couple declarations from stdint.h, but
// C18 doesn't have that header.
#ifndef _INT8_T_DECLARED
typedef char int8_t;
#define _INT8_T_DECLARED
#endif

#ifndef _UINT8_T_DECLARED
typedef unsigned char uint8_t;
#define _UINT8_T_DECLARED
#endif

#ifndef _INT16_T_DECLARED
typedef short int16_t;
#define _INT16_T_DECLARED
#endif

#ifndef _UINT16_T_DECLARED
typedef unsigned short uint16_t;
#define _UINT16_T_DECLARED
#endif


#define OPEN8055_HID_MESSAGE_SIZE	32


typedef union {
	uint8_t msgType;
	
	struct {
		uint8_t			_msgType;
				
		uint8_t			inputBits;
		uint16_t		inputCounter[5];
		uint16_t		adcValue[2];
	} input;
	
	struct {
		uint8_t			_msgType;
		
		uint8_t			outputBits;
		uint16_t		outputValue[8];
		uint16_t		pwmValue[2];
	} output;	
	
	uint8_t				raw[OPEN8055_HID_MESSAGE_SIZE];
} open8055HIDMessage_t;	


#endif // OPEN8055_HID_PROTOCOL_H
