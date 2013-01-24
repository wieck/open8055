#ifndef OPEN8055_HID_PROTOCOL_H
#define OPEN8055_HID_PROTOCOL_H


// We need a couple declarations from stdint.h, but
// C18 doesn't have that header.

#ifndef _STDINT_H

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

#endif /* _STDINT_H */


#define OPEN8055_HID_MESSAGE_SIZE	32


#define OPEN8055_HID_MESSAGE_OUTPUT	0x01
#define OPEN8055_HID_MESSAGE_INPUT	0x81


typedef union {
	uint8_t				raw[OPEN8055_HID_MESSAGE_SIZE];
	uint8_t				msgType;
	
	struct {
		uint8_t			_msgType_input;
				
		uint8_t			inputBits;
		uint16_t		inputCounter[5];
		uint16_t		inputAdcValue[2];
	};
	
	struct {
		uint8_t			_msgType;
		
		uint8_t			outputBits;
		uint16_t		outputValue[8];
		uint16_t		outputPwmValue[2];
	};	
	
} Open8055_hidMessage_t;	


#endif // OPEN8055_HID_PROTOCOL_H
