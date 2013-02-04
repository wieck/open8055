/* ----------------------------------------------------------------------
 * open8055.c
 *
 *	This is an attempt to create a portable library to
 *	access an Open8055 USB Experiment Interface Card.
 *	The code uses libusb 1.0 calls under Unix and native
 *	setupapi calls under Windows.
 *
 * ----------------------------------------------------------------------
 *
 *  Copyright (c) 2013, Jan Wieck
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *      * Neither the name of the <organization> nor the
 *        names of its contributors may be used to endorse or promote products
 *        derived from this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *  
 * ----------------------------------------------------------------------
 */

#include "open8055_compat.h"
#include "open8055.h"
#include "open8055_hid_protocol.h"


/* ----------------------------------------------------------------------
 * Local definitions
 * ----------------------------------------------------------------------
 */

typedef struct {
    int				isLocal;
    int				idLocal;
    char			destination[1024];

    int				error;
    char			errorMessage[1024];

    Open8055_hidMessage_t	currentConfig1;
    Open8055_hidMessage_t	currentOutput;
    Open8055_hidMessage_t	currentInput;
    int				currentInputUnconsumed;

    int				autoFlush;
    int				pendingConfig1;
    int				pendingOutput;

#ifdef _WIN32
    unsigned char		writeBuffer[OPEN8055_HID_MESSAGE_SIZE + 1];
    unsigned char		readBuffer[OPEN8055_HID_MESSAGE_SIZE + 1];
    HANDLE			readEvent;
    int				readPending;
    OVERLAPPED			readOverlapped;
#endif

} Open8055_card_t;


/* ----------------------------------------------------------------------
 * Local functions
 * ----------------------------------------------------------------------
 */
static int Open8055_Init(void);
static void SetError(Open8055_card_t *card, char *fmt, ...);
static int Open8055_WaitForInput(Open8055_card_t *card, int mask, int timeout);

static int DeviceInit(void);
static int DevicePresent(int cardNumber);
static int DeviceOpen(Open8055_card_t *card);
static int DeviceClose(Open8055_card_t *card);
static int DeviceRead(Open8055_card_t *card, void *buffer, int timeout);
static int DeviceWrite(Open8055_card_t *card, void *buffer);
static char *ErrorString(void);


/* ----------------------------------------------------------------------
 * Local data
 * ----------------------------------------------------------------------
 */
static char     		lastErrorMessage[1024] = {'\0'};
static int      		initialized = FALSE;

static int			openLocalCards[OPEN8055_MAX_CARDS];


/* ----------------------------------------------------------------------
 * Macros
 * ----------------------------------------------------------------------
 */
#define CARD_VALID(_c)							\
	if ((_c) == NULL)						\
	{								\
	    SetError(NULL, "Invalid card handle");			\
	    return -1;							\
	}


/* ----------------------------------------------------------------------
 * Public API functions follow
 * ----------------------------------------------------------------------
 */


/* ----
 * Open8055_LastError()
 *
 *  Returns the last error message, or an empty string if
 *  there never was an error.
 * ----
 */
OPEN8055_EXTERN char * STDCALL
Open8055_LastError(OPEN8055_HANDLE h)
{
    Open8055_card_t	*card = (Open8055_card_t *)h;

    if (card == NULL)
    	return lastErrorMessage;

    return card->errorMessage;
}


/* ----
 * Open8055_CardPresent()
 *
 *  Checks if a given card number is present in the local system.
 *  Returns 1 if card is present, 0 if not.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_CardPresent(int cardNumber)
{
    if (!initialized)
    {
    	if (Open8055_Init() < 0)
	    return -1;
    }

    return DevicePresent(cardNumber);
}


/* ----
 * Open8055_Connect()
 *
 *  Attempts to open a local or remote Open8055 card.
 *  Returns -1 on error or the ID of the new card.
 * ----
 */
OPEN8055_EXTERN OPEN8055_HANDLE STDCALL
Open8055_Connect(char *destination, char *password)
{
    int				cardNumber;
    Open8055_card_t	       *card;
    Open8055_hidMessage_t	outputMessage;
    Open8055_hidMessage_t	inputMessage;

    /* ----
     * Make sure the library is initialized.
     * ----
     */
    if (!initialized)
    {
    	if (Open8055_Init() < 0)
	    return NULL;
    }

    /* ----
     * Parse the destination. At some point in the future this string
     * will look like a URI in the format of open8055://user@host/card0-n.
     * For now we just parse the "card0-n" part.
     * ----
     */
    if (sscanf(destination, "card%d", &cardNumber) != 1)
    {
    	SetError(NULL, "Syntax error in local card address '%s'", destination);
	return NULL;
    }

    /* ----
     * Check the card number for validity and make sure it isn't open yet.
     * ----
     */
    if (cardNumber < 0 || cardNumber >= OPEN8055_MAX_CARDS)
    {
    	SetError(NULL, "Card number %d out of bounds", cardNumber);
	return NULL;
    }
    if (openLocalCards[cardNumber] != 0)
    {
    	SetError(NULL, "Local card %d already open", cardNumber);
	return NULL;
    }

    /* ----
     * Allocate the card status data.
     * ----
     */
    card = (Open8055_card_t *)malloc(sizeof(Open8055_card_t));
    if (card == NULL)
    {
    	SetError(NULL, "out of memory");
	return NULL;
    }
    memset(card, 0, sizeof(Open8055_card_t));
    strncpy(card->destination, destination, sizeof(card->destination));
    card->isLocal	= TRUE;
    card->idLocal	= cardNumber;
    card->autoFlush	= TRUE;

    /* ----
     * Try to open the actual local card.
     * ----
     */
    if (DeviceOpen(card) < 0)
    {
	strncpy(lastErrorMessage, card->errorMessage, sizeof(lastErrorMessage));
	free(card);
    	return NULL;
    }

    /* ----
     * Query current card status.
     * ----
     */
    memset(&outputMessage, 0, sizeof(outputMessage));
    outputMessage.msgType = OPEN8055_HID_MESSAGE_GETCONFIG;
    if (DeviceWrite(card, &outputMessage) < 0)
    {
    	SetError(NULL, "Sending of GETCONFIG failed - %s", ErrorString());
	DeviceClose(card);
	free(card);
	return NULL;
    }
    while (card->currentConfig1.msgType == 0x00 || card->currentOutput.msgType == 0x00
    	|| card->currentInput.msgType == 0x00)
    {
    	if (DeviceRead(card, &inputMessage, 0) < 0)
	{
	    SetError(NULL, "DeviceRead failed - %s", ErrorString());
	    DeviceClose(card);
	    free(card);
	    return NULL;
	}
	switch(inputMessage.msgType)
	{
	    case OPEN8055_HID_MESSAGE_SETCONFIG1:
	    	memcpy(&(card->currentConfig1), &inputMessage, sizeof(card->currentConfig1));
	    	break;

	    case OPEN8055_HID_MESSAGE_OUTPUT:
	    	memcpy(&(card->currentOutput), &inputMessage, sizeof(card->currentOutput));
	    	break;

	    case OPEN8055_HID_MESSAGE_INPUT:
	    	card->currentInputUnconsumed = OPEN8055_INPUT_ANY;
	    	memcpy(&(card->currentInput), &inputMessage, sizeof(card->currentInput));
	    	break;
	}
    }

    /* ----
     * Success.
     * ----
     */
    return (OPEN8055_HANDLE)card;
}


/* ----
 * Open8055_Close()
 *
 *  Close an Open8055.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_Close(OPEN8055_HANDLE h)
{
    Open8055_card_t		*card = (Open8055_card_t *)h;
    int				rc = 0;

    CARD_VALID(card);

    /* ----
     * Close the device and free the card structure.
     * ----
     */
    if (DeviceClose(card) < 0)
    {
        strncpy(lastErrorMessage, card->errorMessage, sizeof(lastErrorMessage));
	rc = -1;
    }
    free(card);

    return rc;
}


/* ----
 * Open8055_Reset()
 *
 *  Perform a device Reset. Since the device is supposed to disconnect
 *  we also need to close it.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_Reset(OPEN8055_HANDLE h)
{
    Open8055_card_t		*card = (Open8055_card_t *)h;
    int				rc = 0;
    Open8055_hidMessage_t	message;

    CARD_VALID(card);

    /* ----
     * Send it the RESET command.
     * ----
     */
    memset(&message, 0, sizeof(message));
    message.msgType = OPEN8055_HID_MESSAGE_RESET;
    if (DeviceWrite(card, &message) < 0)
    {
        strncpy(lastErrorMessage, card->errorMessage, sizeof(lastErrorMessage));
	rc = -1;
    }

    /* ----
     * Close the card and free all data.
     * ----
     */
    if (DeviceClose(card) < 0)
    {
        strncpy(lastErrorMessage, card->errorMessage, sizeof(lastErrorMessage));
	rc = -1;
    }
    free(card);

    return rc;
}


/* ----
 * Open8055_Wait()
 *
 *  Wait until any new input becomes available
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_Wait(OPEN8055_HANDLE h, int timeout)
{
    Open8055_card_t	*card = (Open8055_card_t *)h;

    return Open8055_WaitForInput(card, OPEN8055_INPUT_ANY, timeout);
}


/* ----
 * Open8055_WaitFor()
 *
 *  Wait until specific input ports have changed.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_WaitFor(OPEN8055_HANDLE h, int mask, int timeout)
{
    Open8055_card_t	*card = (Open8055_card_t *)h;

    return Open8055_WaitForInput(card, mask, timeout);
}


/* ----
 * Open8055_GetAutoFlush()
 *
 *  Return the current autoFlush setting.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_GetAutoFlush(OPEN8055_HANDLE h)
{
    Open8055_card_t	*card = (Open8055_card_t *)h;

    if(card->autoFlush)
    	return 1;

    return 0;
}


/* ----
 * Open8055_SetAutoFlush()
 *
 *  Set the auto flush feature.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_SetAutoFlush(OPEN8055_HANDLE h, int flag)
{
    Open8055_card_t	*card = (Open8055_card_t *)h;

    card->autoFlush = (flag == TRUE);

    if (card->autoFlush)
    	return Open8055_Flush(h);

    return 0;
}


/* ----
 * Open8055_Flush()
 *
 *  Send pending changes to the card.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_Flush(OPEN8055_HANDLE h)
{
    Open8055_card_t	*card = (Open8055_card_t *)h;
    int			rc = 0;

    if (card->pendingConfig1)
    {
    	if (DeviceWrite(card, &(card->currentConfig1)) < 0)
	{
	    return -1;
	}
	card->pendingConfig1 = FALSE;
    }

    if (card->pendingOutput)
    {
    	if (DeviceWrite(card, &(card->currentOutput)) < 0)
	{
	    return -1;
	}
	card->pendingOutput = FALSE;
	card->currentOutput.resetCounter = 0x00;
    }

    return rc;
}


/* ----
 * Open8055_GetInputDigitalAll()
 *
 *  Read the 5 digital input ports
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_GetInputDigitalAll(OPEN8055_HANDLE h)
{
    Open8055_card_t	*card = (Open8055_card_t *)h;
    int			rc = 0;

    /* ----
     * Make sure we have current input data.
     * ----
     */
    Open8055_WaitForInput(card, OPEN8055_INPUT_I_ANY, OPEN8055_WAITFOR_US);

    /* ----
     * Stop here on error.
     * ----
     */
    if (card->error)
    {
	return -1;
    }

    /* ----
     * Mark all digital inputs as consumed and return the current state.
     * ----
     */
    card->currentInputUnconsumed &= ~OPEN8055_INPUT_I_ANY;
    rc = card->currentInput.inputBits;

    return rc;
}


/* ----
 * Open8055_GetInputADC()
 *
 *  Read the current value of an ADC
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_GetInputADC(OPEN8055_HANDLE h, int port)
{
    Open8055_card_t	*card = (Open8055_card_t *)h;
    int			rc = 0;

    if (port < 0 || port > 2)
    {
    	SetError(card, "invalid port number %d", port);
	return -1;
    }

    /* ----
     * Make sure we have current input data.
     * ----
     */
    Open8055_WaitForInput(card, OPEN8055_INPUT_ADC1 << port, OPEN8055_WAITFOR_US);

    /* ----
     * If the card is in error state, just copy it's error message and 
     * return with error.
     * ----
     */
    if (card->error)
    {
	return -1;
    }

    /* ----
     * Mark the counter consumed.
     * ----
     */
    card->currentInputUnconsumed &= ~(OPEN8055_INPUT_ADC1 << port);
    rc = ntohs(card->currentInput.inputAdcValue[port]);

    return rc;
}


/* ----
 * Open8055_GetInputCounter()
 *
 *  Read the current value of a counter.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_GetInputCounter(OPEN8055_HANDLE h, int port)
{
    Open8055_card_t	*card = (Open8055_card_t *)h;
    int			rc = 0;

    if (port < 0 || port > 5)
    {
    	SetError(card, "invalid port number %d", port);
	return -1;
    }

    /* ----
     * Make sure we have current input data.
     * ----
     */
    Open8055_WaitForInput(card, OPEN8055_INPUT_COUNT1 << port, OPEN8055_WAITFOR_US);

    /* ----
     * If the card is in error state stop here.
     * ----
     */
    if (card->error)
    {
	return -1;
    }

    /* ----
     * Mark the counter consumed.
     * ----
     */
    card->currentInputUnconsumed &= ~(OPEN8055_INPUT_COUNT1 << port);
    rc = ntohs(card->currentInput.inputCounter[port]);

    return rc;
}


/* ----
 * Open8055_ResetInputCounter()
 *
 *  Reset an individual input counter.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_ResetInputCounter(OPEN8055_HANDLE h, int port)
{
    Open8055_card_t	*card = (Open8055_card_t *)h;
    int			rc = 0;

    if (port < 0 || port > 5)
    {
    	SetError(card, "invalid value for port");
	return -1;
    }

    /* ----
     * Set the value and send it if in autoFlush mode.
     * ----
     */
    card->currentOutput.resetCounter |= (1 << port);
    if (card->autoFlush)
    {
	if (DeviceWrite(card, &(card->currentOutput)) < 0)
	    rc = -1;
    	card->pendingOutput = FALSE;
    }
    else
    {
	card->pendingOutput = TRUE;
    }

    return rc;
}


/* ----
 * Open8055_GetInputDebounce()
 *
 *  Set the debounce value of a digital input.
 * ----
 */
OPEN8055_EXTERN double STDCALL
Open8055_GetInputDebounce(OPEN8055_HANDLE h, int port)
{
    Open8055_card_t	*card = (Open8055_card_t *)h;
    double		rc;

    if (port < 0 || port > 5)
    {
    	SetError(card, "invalid value for port");
	return -1;
    }

    rc = (double)ntohs(card->currentConfig1.debounceValue[port]) / 10.0;

    return rc;
}


/* ----
 * Open8055_SetInputDebounce()
 *
 *  Set the debounce value of a digital input.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_SetInputDebounce(OPEN8055_HANDLE h, int port, double ms)
{
    Open8055_card_t	*card = (Open8055_card_t *)h;
    int			rc = 0;

    if (port < 0 || port > 5)
    {
    	SetError(card, "invalid value for port");
	return -1;
    }
    if (ms < 0.0 || ms > 5000.0)
    {
    	SetError(card, "invalid value for ms");
	return -1;
    }

    /* ----
     * Set the value and send it if in autoFlush mode.
     * ----
     */
    card->currentConfig1.debounceValue[port] = htons((uint16_t)floor(ms * 10.0));
    if (card->autoFlush)
    {
	if (DeviceWrite(card, &(card->currentConfig1)) < 0)
	    rc = -1;
    	card->pendingConfig1 = FALSE;
    }
    else
    {
	card->pendingConfig1 = TRUE;
    }

    return rc;
}


/* ----
 * Open8055_ResetInputCounter()
 *
 *  Reset an individual input counter.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_ResetInputCounterAll(OPEN8055_HANDLE h)
{
    Open8055_card_t	*card = (Open8055_card_t *)h;
    int			rc = 0;

    /* ----
     * Set the value and send it if in autoFlush mode.
     * ----
     */
    card->currentOutput.resetCounter = 0x1F;
    if (card->autoFlush)
    {
	if (DeviceWrite(card, &(card->currentOutput)) < 0)
	    rc = -1;
    	card->pendingOutput = FALSE;
	card->currentOutput.resetCounter = 0x00;
    }
    else
    {
	card->pendingOutput = TRUE;
    }

    return rc;
}


/* ----
 * Open8055_GetOutputDigitalAll()
 *
 *  Return the current digital output settings.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_GetOutputDigitalAll(OPEN8055_HANDLE h)
{
    Open8055_card_t	*card = (Open8055_card_t *)h;
    int			rc = 0;

    /* ----
     * We have queried them at Connect and tracked them all the time.
     * ----
     */
    rc = card->currentOutput.outputBits;

    return rc;
}



/* ----
 * Open8055_GetOutputPWM()
 *
 *  Return the current setting of a PWM output
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_GetOutputPWM(OPEN8055_HANDLE h, int port)
{
    Open8055_card_t	*card = (Open8055_card_t *)h;
    int			rc = 0;

    if (port < 0 || port > 1)
    {
    	SetError(card, "invalid value for port");
	return -1;
    }

    /* ----
     * We have queried them at Connect and tracked them all the time.
     * ----
     */
    rc = ntohs(card->currentOutput.outputPwmValue[port]);

    return rc;
}


/* ----
 * Open8055_SetOutputDigitalAll()
 *
 *  Change all 8 digital outputs.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_SetOutputDigitalAll(OPEN8055_HANDLE h, int bits)
{
    Open8055_card_t	*card = (Open8055_card_t *)h;
    int			rc = 0;

    if (bits < 0 || bits > 255)
    {
    	SetError(card, "invalid value for bits");
	return -1;
    }

    /* ----
     * Set the bits and send them if in autoFlush mode.
     * ----
     */
    card->currentOutput.outputBits = bits;
    if (card->autoFlush)
    {
	if (DeviceWrite(card, &(card->currentOutput)) < 0)
	    rc = -1;
    	card->pendingOutput = FALSE;
    }
    else
    {
	card->pendingOutput = TRUE;
    }

    return rc;
}


/* ----
 * Open8055_SetOutputPWM()
 *
 *  Change one of the PWM outputs.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_SetOutputPWM(OPEN8055_HANDLE h, int port, int value)
{
    Open8055_card_t	*card = (Open8055_card_t *)h;
    int			rc = 0;

    if (port < 0 || port > 1)
    {
    	SetError(card, "invalid value for port");
	return -1;
    }
    if (value < 0 || value > 1023)
    {
    	SetError(card, "invalid value for value");
	return -1;
    }

    /* ----
     * Set the new PWM value and send it if in autoFlush mode.
     * ----
     */
    card->currentOutput.outputPwmValue[port] = htons(value);
    if (card->autoFlush)
    {
	if (DeviceWrite(card, &(card->currentOutput)) < 0)
	    rc = -1;
    	card->pendingOutput = FALSE;
    }
    else
    {
	card->pendingOutput = TRUE;
    }

    return rc;
}



/* ----------------------------------------------------------------------
 * Local functions follow
 * ----------------------------------------------------------------------
 */


static int
Open8055_Init(void)
{
    if (initialized)
    	return 0;

    if (DeviceInit() < 0)
    {
	return -1;
    }

    initialized = TRUE;
    return 0;
}


/* ----
 * Open8055_WaitForInput()
 *
 *  Wait until specific input ports have changed.
 * ----
 */
static int
Open8055_WaitForInput(Open8055_card_t *card, int mask, int timeout)
{
    Open8055_hidMessage_t	inputMessage;
    int				rc = 0;

    /* ----
     * Check for previous error condition.
     * ----
     */
    if (card->error)
    	return -1;

    /* ----
     * Check if we currently have that data unconsumed.
     * ----
     */
    if ((card->currentInputUnconsumed & mask) != 0)
    	return 1;

    /* ----
     * Need to read a new HID report.
     * ----
     */
    rc = DeviceRead(card, &inputMessage, timeout);
    if (rc <= 0)
    	return rc;

    /* ----
     * Handle by message type.
     * ----
     */
    switch (inputMessage.msgType)
    {
	case OPEN8055_HID_MESSAGE_INPUT:
	    memcpy(&(card->currentInput), &inputMessage, sizeof(card->currentInput));
	    card->currentInputUnconsumed = OPEN8055_INPUT_ANY;
	    break;

	case OPEN8055_HID_MESSAGE_SETCONFIG1:
	case OPEN8055_HID_MESSAGE_OUTPUT:
	    break;

	default:
	    SetError(card, "Received unknown message type 0x%02x", inputMessage.msgType);
	    rc = -1;
    }

    return rc;
}


/* ----
 * SetError()
 *
 *  Save an error message in the lastErrorMessage buffer.
 * ----
 */
static void
SetError(Open8055_card_t *card, char *fmt, ...)
{
    va_list     ap;

    va_start(ap, fmt);
    if (card == NULL)
    {
	vsnprintf(lastErrorMessage, sizeof(lastErrorMessage), fmt, ap);
    }
    else
    {
    	card->error = TRUE;
	vsnprintf(card->errorMessage, sizeof(card->errorMessage), fmt, ap);
    }
    va_end(ap);
}


#if 0
/* ----
 * CardIOThread()
 *
 *  This function implements the physical IO with an Open8055 card.
 * ----
 */
static void *
CardIOThread(void *cdata)
{
    Open8055_card_t		*card = (Open8055_card_t *)cdata;
    Open8055_hidMessage_t	inputMessage;
    int				rc;

    pthread_mutex_lock(&(card->cardLock));

    while(TRUE)
    {
	/* ----
	 * Check for card terminate flag
	 * ----
	 */
	if (card->readerTerminate)
	{
	    pthread_mutex_unlock(&(card->cardLock));
	    return NULL;
	}

	/* ----
	 * Wait for a message from the card. Unlock the card status data
	 * so other threads can access it.
	 * ----
	 */
	pthread_mutex_unlock(&(card->cardLock));
	rc = DeviceRead(card, &inputMessage, sizeof(inputMessage));
	pthread_mutex_lock(&(card->cardLock));

	/* ----
	 * Check for physical card IO error.
	 * ----
	 */
	if (rc < 0)
	{
	    /* ----
	     * In case of error signal all possible IO waiters and terminate the IO thread.
	     * ----
	     */
	    pthread_cond_broadcast(&(card->readWaiterCond));
	    pthread_mutex_unlock(&(card->cardLock));
	    return NULL;
	}

	switch (inputMessage.msgType)
	{
	    case OPEN8055_HID_MESSAGE_INPUT:
		memcpy(&(card->currentInput), &inputMessage, sizeof(card->currentInput));
		card->currentInputUnconsumed = OPEN8055_INPUT_ANY;
		if (card->readWaiters > 0)
		    pthread_cond_broadcast(&(card->readWaiterCond));
		break;

	    case OPEN8055_HID_MESSAGE_SETCONFIG1:
	    case OPEN8055_HID_MESSAGE_OUTPUT:
	    	break;

	    default:
		SetError(card, "Received unknown message type 0x%02x", inputMessage.msgType);
			
		pthread_cond_broadcast(&(card->readWaiterCond));
		pthread_mutex_unlock(&(card->cardLock));
		return NULL;
	}
    }

    return NULL;
}
#endif


/* ----------------------------------------------------------------------
 * OS specific USB IO code follows
 * ----------------------------------------------------------------------
 */


#ifdef _WIN32

/* ------------------------------------------------------
 * Following is the Windows specific USB access code.
 * ------------------------------------------------------
 */


/* ----
 * Windows specific functions.
 * ----
 */
static char *DeviceFindPath(int cardNumber);


/* ----
 * Local data
 * ----
 */
static HANDLE   cardHandleRecv[OPEN8055_MAX_CARDS];
static HANDLE   cardHandleSend[OPEN8055_MAX_CARDS];


/* ----
 * DeviceInit()
 *
 *  No initializations required under Windows.
 * ----
 */
static int
DeviceInit(void)
{
    return 0;
}


/* ----
 * DevicePresent() 
 *
 *  Check if a given Open8055 card is present in the system without
 *  actually opening it.
 * ----
 */
static int
DevicePresent(int cardNumber)
{
    char       *devicePath;

    /* ----
     * Make sure the library is initialized.
     * ----
     */
    if ((devicePath = DeviceFindPath(cardNumber)) == NULL)
	return 0;

    free(devicePath);
    return 1;
}


/* ----
 * DeviceOpen()
 *
 *  Open a specific Open8055 by card number.
 * ----
 */
static int
DeviceOpen(Open8055_card_t *card)
{
    char           *path;

    /* ----
     * Lookup the device path for the requested card.
     * ----
     */
    path = DeviceFindPath(card->idLocal);
    if (path == NULL)
    {
	SetError(card, "Open8055 card number %d not present", card->idLocal);
	return -1;
    }

    /* ----
     * Try to open the device.
     * ----
     */
    cardHandleRecv[card->idLocal] = CreateFile(path, GENERIC_READ, 
	    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 
	    FILE_FLAG_OVERLAPPED, 0);
    if (cardHandleRecv[card->idLocal] == INVALID_HANDLE_VALUE)
    {
	SetError(card, "Cannot open Open8055 card number %d - %s", card->idLocal, ErrorString());
	free(path);
	return -1;
    }

    cardHandleSend[card->idLocal] = CreateFile(path, GENERIC_WRITE, 
	    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);

    if (cardHandleSend[card->idLocal] == INVALID_HANDLE_VALUE)
    {
	SetError(card, "Cannot open Open8055 card number %d - %s", card->idLocal, ErrorString());
	free(path);
	return -1;
    }
    free(path);

    /* ----
     * Create the event we need for overlapped IO.
     * ----
     */
    card->readEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (card->readEvent == NULL)
    {
	SetError(card, "Cannot create event object for Open8055 card number %d - %s", card->idLocal, ErrorString());
	return -1;
    }

    /* ----
     * Success.
     * ----
     */
    return 0;
}


/* ----
 * DeviceClose()
 *
 *  Close the specified card.
 * ----
 */
static int
DeviceClose(Open8055_card_t *card)
{
    int     			rc = 0;
    Open8055_hidMessage_t	message;

    /* ----
     * If there is a pending overlapped read, request a forced HID report
     * from the card to get the other thread out of there.
     * ----
     */
    if (card->readPending)
    {
    	SetError(card, "card closed");
	memset(&message, 0, sizeof(message));
	message.msgType = OPEN8055_HID_MESSAGE_GETINPUT;
	DeviceWrite(card, &message);
    }

    /* ----
     * Close all the handles.
     * ----
     */
    if (!CloseHandle(card->readEvent))
    {
	CloseHandle(cardHandleSend[card->idLocal]);
	CloseHandle(cardHandleRecv[card->idLocal]);
	SetError(card, "Cannot close Open8055 card number %d - %s", card->idLocal, ErrorString());
	rc = -1;
    }
    if (!CloseHandle(cardHandleRecv[card->idLocal]))
    {
	CloseHandle(cardHandleSend[card->idLocal]);
	SetError(card, "Cannot close Open8055 card number %d - %s", card->idLocal, ErrorString());
	rc = -1;
    }
    if (!CloseHandle(cardHandleSend[card->idLocal]))
    {
	SetError(card, "Cannot close Open8055 card number %d - %s", card->idLocal, ErrorString());
	rc = -1;
    }

    return rc;
}


/* ----
 * DeviceRead()
 *
 *  Receive one message from an Open8055.
 * ----
 */
static int
DeviceRead(Open8055_card_t *card, void *buffer, int timeout)
{
    unsigned char      *ioBuf = card->readBuffer;
    DWORD       	bytesRead;

    if (!card->readPending)
    {
	/* ----
	 * We don't have a pending read. Issue an OVERLAPPED ReadFile().
	 * ----
	 */
	memset(&(card->readOverlapped), 0, sizeof(card->readOverlapped));
	card->readOverlapped.hEvent = card->readEvent;
	if (!ReadFile(cardHandleRecv[card->idLocal], ioBuf, OPEN8055_HID_MESSAGE_SIZE + 1,
		&bytesRead, &(card->readOverlapped)))
	{
	    /* ----
	     * Anything other than ERROR_IO_PENDING is a real IO error.
	     * ----
	     */
	    if (GetLastError() != ERROR_IO_PENDING)
	    {
		SetError(card, "ReadFile() failed for card %d - %s", card->idLocal, ErrorString());
		return -1;
	    }	

	    /* ----
	     * This means that the request is actually pending.
	     * ----
	     */
	    card->readPending = TRUE;
	}
    }

    if (card->readPending)
    {
	switch(WaitForSingleObject(card->readEvent, (timeout == 0) ? INFINITE : timeout))
	{
	    case WAIT_OBJECT_0:
		ResetEvent(card->readEvent);
	    	break;

	    case WAIT_TIMEOUT:
		return 0;
	}
    }

    if (card->readPending)
    {
	card->readPending = FALSE;
    	if (!GetOverlappedResult(cardHandleRecv[card->idLocal], &(card->readOverlapped),
		&bytesRead, TRUE))
	{
	    SetError(card, "GetOverlappedResult() failed for card %d - %s", card->idLocal, ErrorString());
	    return -1;
	}
    }

    if (bytesRead != OPEN8055_HID_MESSAGE_SIZE + 1)
    {
	SetError(card, "Short read from card %d - expected %d but got %d", 
		card->idLocal, OPEN8055_HID_MESSAGE_SIZE + 1, bytesRead);
	return -1;
    }

    memcpy(buffer, &ioBuf[1], OPEN8055_HID_MESSAGE_SIZE);

    return 1;
}


/* ----
 * DeviceWrite()
 *
 *  Send one message to the Open8055.
 * ----
 */
static int
DeviceWrite(Open8055_card_t *card, void *buffer)
{
    unsigned char      *ioBuf = card->writeBuffer;
    DWORD       	bytesWritten;

    ioBuf[0] = '\0';
    memcpy(&ioBuf[1], buffer, OPEN8055_HID_MESSAGE_SIZE);

    if (!WriteFile(cardHandleSend[card->idLocal], ioBuf, OPEN8055_HID_MESSAGE_SIZE + 1, &bytesWritten, NULL))
    {
	SetError(card, "WriteFile() failed for card %d - %s", card->idLocal, ErrorString());
	free(ioBuf);
	return -1;
    }

    if (bytesWritten != OPEN8055_HID_MESSAGE_SIZE + 1)
    {
	SetError(card, "Short write to card %d - expected %d but wrote %d", 
		card->idLocal, OPEN8055_HID_MESSAGE_SIZE + 1, bytesWritten);
	return -1;
    }

    return 1;
}

/* ----
 * DeviceFindPath()
 *
 *  Searches for the device path of a specific Open8055 card.
 *  Returns the device path of the card or NULL if no such card is found.
 *  It is the caller's responsibility to free the allocated string.
 * ----
 */
static char *
DeviceFindPath(int cardNumber)
{
    HDEVINFO                    DeviceInfoSet;
    SP_DEVICE_INTERFACE_DATA    DeviceInterfaceData;
    DWORD                       index = 0;
    GUID                        open8055_Guid;
    char                        vidPidString[64];
    char                       *result = NULL;

    /* ----
     * Parse the GUID string into the internal data format.
     * ----
     */
    if (UuidFromString((unsigned char *)OPEN8055_GUID, &open8055_Guid) != RPC_S_OK)
    {
	SetError(NULL, "UuidFromString() failed");
	return NULL;
    }

    /* ----
     * Build the magic string we are looking for in the device paths.
     * ----
     */
    snprintf(vidPidString, sizeof(vidPidString), "vid_10cf&pid_%04x", OPEN8055_PID + cardNumber);

    /* ----
     * Get the device info set for this GUID's class.
     * ----
     */
    DeviceInfoSet = SetupDiGetClassDevs(&open8055_Guid, NULL, 0,
	    DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
	SetError(NULL, "SetupDiGetClassDevs(): error %ld", GetLastError());
	return NULL;
    }

    /* ----
     * Enumerate that set and look for the requested card.
     * ----
     */
    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    while (SetupDiEnumDeviceInterfaces(DeviceInfoSet, NULL, &open8055_Guid,
	    index++, &DeviceInterfaceData))
    {
	PSP_DEVICE_INTERFACE_DETAIL_DATA    DetailData;
	DWORD                               RequiredSize;

	/* ----
	 * Get the required size for the device interface detail data.
	 * ----
	 */
	SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, &DeviceInterfaceData,
		NULL, 0, &RequiredSize, NULL);

	/* ----
	 * Allocate the detail data structure in the required size.
	 * ----
	 */
	DetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(RequiredSize + sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA));
	if (DetailData == NULL)
	{
	    SetError(NULL, "malloc(): out of memory");
	    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	    return NULL;
	}
	DetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

	/* ----
	 * Now get the actual interface detail.
	 * ----
	 */
	if (!SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, &DeviceInterfaceData,
		DetailData, RequiredSize, &RequiredSize, NULL))
	{
	    SetError(NULL, "SetupDiGetInterfaceDetail(): error %ld", GetLastError());
	    free(DetailData);
	    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	    return NULL;
	}

	/* ----
	 * Check if this is the device we are looking for. If so copy the
	 * device path as result and stop the search.
	 * ----
	 */
	if (strstr(DetailData->DevicePath, vidPidString) != NULL)
	{
	    result = strdup(DetailData->DevicePath);
	    free(DetailData);
	    break;
	}

	/* ----
	 * Not the one we are looking for. Free allocated memory.
	 * ----
	 */
	free(DetailData);
    }

    /* ----
     * Free the device info list and return the result of our search.
     * ----
     */
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    return result;
}


/* ----
 * ErrorString()
 *
 *  Get the error message that corresponds to GetLastError().
 * ----
 */
static char *
ErrorString(void)
{
    static char errStr[1024];

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
	    NULL, GetLastError(), 
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	    errStr, sizeof(errStr), NULL);

    return errStr;
}



#else /* _WIN32 */

/* ------------------------------------------------------
 * Following is the Unix specific USB access code. 
 * ------------------------------------------------------
 */


/* ----
 * Local data
 * ----
 */
static libusb_context          *libusbCxt;
static libusb_device_handle    *cardHandle[OPEN8055_MAX_CARDS];
static int                      hadKernelDriver[OPEN8055_MAX_CARDS];


/* ----
 * DeviceInit()
 *
 *  Initialize libusb.
 * ----
 */
static int
DeviceInit(void)
{
    if (libusb_init(&libusbCxt) != 0)
    {
	SetError("libusb_init() failed - %s", ErrorString());
	return -1;
    }

    return 0;
}


/* ----
 * DevicePresent()
 *
 *  Check if a given Open8055 card is present in the system without
 *  actually opening it.
 * ----
 */
static int
DevicePresent(int cardNumber)
{
    int                         cardFound = 0;
    int                         numDevices;
    libusb_device             **deviceList;
    libusb_device_descriptor    deviceDesc;
    int                         i;

    /* ----
     * Get the list of USB devices in the system.
     * ----
     */
    numDevices = libusb_get_device_list(libusbCxt, &deviceList);
    if (numDevices < 0)
    {
	SetError("libusb_get_device_list(): %s", ErrorString());
	return -1;
    }

    /* ----
     * See if any of them is the Open8055 we are looking for.
     * ----
     */
    for (i = 0; i < numDevices; i++)
    {
	if (libusb_get_device_descriptor(deviceList[i], &deviceDesc) != 0)
	{
	    SetError("libusb_get_device_descriptor(): %s", ErrorString());
	    libusb_free_device_list(deviceList, 1);
	    return -1;
	}
	
	if (deviceDesc.idVendor == OPEN8055_VID &&
	    deviceDesc.idProduct == OPEN8055_PID + cardNumber)
	{
	    cardFound = 1;
	    break;
	}
    }

    /* ----
     * Cleanup and return result.
     * ----
     */
    libusb_free_device_list(deviceList, 1);

    return cardFound;
}


/* ----
 * DeviceOpen()
 *
 *  Open a specific Open8055 by card number.
 * ----
 */
static int
DeviceOpen(int cardNumber)
{
    libusb_device_handle   *dev;
    int                     rc;
    int                     interface = 0;

    /* ----
     * Open the device.
     * ----
     */
    dev = cardHandle[cardNumber] = libusb_open_device_with_vid_pid(
	    libusbCxt, OPEN8055_VID, OPEN8055_PID + cardNumber);
    if (cardHandle[cardNumber] == NULL)
    {
	SetError("libusb_open_device_with_vid_pid(): %s", ErrorString());
	return -1;
    }

    /* ----
     * Set configuration
     * ----
     */
    if (libusb_set_configuration(dev, 1) != 0)
    {
	SetError("libusb_set_configuration(): %s", ErrorString());
	libusb_close(dev);
	return -1;
    }

    /* ----
     * If a kernel driver is active, detach it.
     * ----
     */
    if ((rc = libusb_kernel_driver_active(dev, interface)) < 0)
    {
	SetError("libusb_kernel_driver_active(): %s", ErrorString());
	libusb_close(dev);
	return -1;
    }
    else
    {
	hadKernelDriver[cardNumber] = rc;
	if (rc != 0)
	{
	    if (libusb_detach_kernel_driver(dev, interface) != 0)
	    {
		SetError("libusb_detach_kernel_driver(): %s", ErrorString());
		libusb_close(dev);
		return -1;
	    }
	}
    }

    /* ----
     * Claim the interface.
     * ----
     */
    if (libusb_claim_interface(dev, interface) != 0)
    {
	SetError("libusb_claim_interface(): %s", ErrorString());
	if (hadKernelDriver[cardNumber])
	    libusb_attach_kernel_driver(dev, interface);
	libusb_close(dev);
	return -1;
    }

    /* ----
     * Activate alternate setting
     * ----
     */
    if (libusb_set_interface_alt_setting(dev, interface, 0) != 0)
    {
	SetError("libusb_set_interface_alt_setting(): %s", ErrorString());
	libusb_release_interface(dev, interface);
	if (hadKernelDriver[cardNumber])
	    libusb_attach_kernel_driver(dev, interface);
	libusb_close(dev);
	return -1;
    }

    return 0;
}


/* ----
 * DeviceClose()
 *
 *  Close the specified card.
 * ----
 */
static int
DeviceClose(int cardNumber)
{
    int     interface = 0;

    libusb_release_interface(cardHandle[cardNumber], interface);
    if (hadKernelDriver[cardNumber])
	libusb_attach_kernel_driver(cardHandle[cardNumber], interface);
    libusb_close(cardHandle[cardNumber]);

    return 0;
}


/* ----
 * DeviceRead()
 *
 *  Receive one message from the Open8055.
 * ----
 */
static int
DeviceRead(int cardNumber, unsigned char *buffer, int len)
{
    int     bytesRead;

    if (libusb_interrupt_transfer(cardHandle[cardNumber],
	    LIBUSB_ENDPOINT_IN | 1, (void *)buffer, 
	    len, &bytesRead, 0) == 0)
    {
	if (bytesRead != len)
	{
	    SetError("short read - expected %d, got %d", len, bytesRead);
	    return -1;
	}

	return bytesRead;
    }

    SetError("libusb_interrupt_transfer(): %s", ErrorString());
    return -1;
}


/* ----
 * DeviceWrite()
 *
 *  Send one message to the Open8055.
 * ----
 */
static int
DeviceWrite(int cardNumber, unsigned char *buffer, int len)
{
    int     bytesWritten;

    if (libusb_interrupt_transfer(cardHandle[cardNumber],
	    LIBUSB_ENDPOINT_OUT | 1, (void *)buffer, 
	    len, &bytesWritten, 0) == 0)
    {
	if (bytesWritten != len)
	{
	    SetError("short write - expected %d, wrote %d", len, bytesWritten);
	    return -1;
	}

	return bytesWritten;
    }

    SetError("libusb_interrupt_transfer(): %s", ErrorString());
    return -1;
}


/* ----
 * ErrorString()
 *
 *  Get the error message that corresponds to GetLastError().
 * ----
 */
static char *
ErrorString(void)
{
    static char errStr[1024];

    snprintf(errStr, sizeof(errStr), "%s", strerror(errno));
    return errStr;
}

#endif /* _WIN32 */
