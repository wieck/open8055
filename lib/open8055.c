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

#include <pthread.h>
#include <time.h>


/* ----------------------------------------------------------------------
 * Local definitions
 * ----------------------------------------------------------------------
 */

typedef struct {
    int				isLocal;
    int				idLocal;

    pthread_t			ioThread;
    pthread_mutex_t		lock;
    pthread_cond_t		cond;
    int				ioTerminate;
    int				ioError;
    char			ioErrorMessage[1024];
    int				ioWaiters;

    Open8055_hidMessage_t	currentConfig1;
    Open8055_hidMessage_t	currentOutput;
    Open8055_hidMessage_t	currentInput;
    uint32_t			currentInputUnconsumed;
} Open8055_card_t;


/* ----------------------------------------------------------------------
 * Local functions
 * ----------------------------------------------------------------------
 */
static int Open8055_Init(void);
static void SetError(char *fmt, ...);
static void *CardIOThread(void *cdata);
static int Open8055_WaitForInput(Open8055_card_t *card, uint32_t mask, long us);

static int DeviceInit(void);
static int DevicePresent(int cardNumber);
static int DeviceOpen(int cardNumber);
static int DeviceClose(int cardNumber);
static int DeviceRead(int cardNumber, void *buffer, int len);
static int DeviceWrite(int cardNumber, void *buffer, int len);
static char *ErrorString(void);


/* ----------------------------------------------------------------------
 * Local data
 * ----------------------------------------------------------------------
 */
static char     		lastErrorMessage[1024] = {'\0'};
static pthread_mutex_t		lastErrorLock;
static int      		initialized = FALSE;

static Open8055_card_t	      **openCards = NULL;
static int			openCardsAlloc = 0;
static int			openCardsUsed = 0;
static int			openLocalCards[OPEN8055_MAX_CARDS];
static pthread_mutex_t		openCardsLock;


/* ----------------------------------------------------------------------
 * Macros
 * ----------------------------------------------------------------------
 */
#define OPEN8055_INIT()							\
	if (!initialized)						\
	{								\
	    if (Open8055_Init() < 0)					\
	        return -1;						\
	}

#define OPEN8055_LOCK_CARD(_h, _c)					\
	pthread_mutex_lock(&openCardsLock);				\
	if ((_h) < 0 || (_h) >= openCardsUsed || openCards[(_h)] == NULL) \
	{								\
	    SetError("Invalid card handle %d", (_h));			\
	    return -1;							\
	}								\
	card = openCards[(_h)];						\
	pthread_mutex_lock(&(card->lock));				\
	pthread_mutex_unlock(&openCardsLock);

#define OPEN8055_UNLOCK_CARD(_c)					\
	pthread_mutex_unlock(&((_c)->lock));


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
Open8055_LastError(void)
{
    return lastErrorMessage;
}


/* ----
 * Open8055_LastErrorCopy()
 *
 *  Returns the last error message in allocated memory.
 *  This is thread safe but the caller is responsible to
 *  free the memory.
 * ----
 */
OPEN8055_EXTERN char * STDCALL
Open8055_LastErrorCopy(void)
{
    char *dup;

    if (!initialized)
    	return strdup(lastErrorMessage);

    pthread_mutex_lock(&lastErrorLock);
    dup = strdup(lastErrorMessage);
    pthread_mutex_unlock(&lastErrorLock);

    return dup;
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
    OPEN8055_INIT();
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
OPEN8055_EXTERN int STDCALL
Open8055_Connect(char *destination, char *password)
{
    int				cardNumber;
    int				handle;
    Open8055_card_t	       *card;
    Open8055_hidMessage_t	outputMessage;
    Open8055_hidMessage_t	inputMessage;

    OPEN8055_INIT();

    /* ----
     * Parse the destination. At some point in the future this string
     * will look like a URI in the format of open8055://user@host/card0-n.
     * For now we just parse the "card0-n" part.
     * ----
     */
    if (sscanf(destination, "card%d", &cardNumber) != 1)
    {
    	SetError("Syntax error in local card address '%s'", destination);
	return -1;
    }

    /* ----
     * Check the card number for validity and make sure it isn't open yet.
     * ----
     */
    if (cardNumber < 0 || cardNumber >= OPEN8055_MAX_CARDS)
    {
    	SetError("Card number %d out of bounds", cardNumber);
	return -1;
    }
    if (openLocalCards[cardNumber] != 0)
    {
    	SetError("Local card %d already open", cardNumber);
	return -1;
    }

    /* ----
     * Try to open the actual local card.
     * ----
     */
    if (DeviceOpen(cardNumber) < 0)
    {
    	return -1;
    }

    /* ----
     * Find an unused handle slot.
     * ----
     */
    pthread_mutex_lock(&openCardsLock);
    if (openCardsAlloc == 0)
    {
	/* ----
	 * On first call allocate an initial 16 slots.
	 * ----
	 */
    	openCards = (Open8055_card_t **)malloc(sizeof(Open8055_card_t*) * 16);
	if (openCards == NULL)
	{
	    SetError("out of memory");
	    DeviceClose(cardNumber);
	    pthread_mutex_unlock(&openCardsLock);
	    return -1;
	}
	openCardsAlloc = 16;
    }
    for (handle = 0; handle < openCardsUsed; handle++)
    	if (openCards[handle] == NULL) break;

    if (handle == openCardsUsed)
    {
    	/* ----
	 * No previously free'd slot found. See if we have spare
	 * allocated slots.
	 * ----
	 */
    	if (openCardsUsed == openCardsAlloc)
	{
	    /* ----
	     * Ran out of slots. Double the table size.
	     * ----
	     */
	    openCards = (Open8055_card_t **)realloc(openCards, sizeof(Open8055_card_t*) * openCardsAlloc * 2);
	    if (openCards == NULL)
	    {
	        SetError("out of memory");
		DeviceClose(cardNumber);
		pthread_mutex_unlock(&openCardsLock);
		return -1;
	    }
	    openCardsAlloc *= 2;
	}

	/* ----
	 * handle is now going to use the next slot.
	 * ----
	 */
	openCardsUsed++;
    }

    /* ----
     * Allocate the card status data.
     * ----
     */
    card = openCards[handle] = (Open8055_card_t *)malloc(sizeof(Open8055_card_t));
    if (card == NULL)
    {
    	SetError("out of memory");
	DeviceClose(cardNumber);
	pthread_mutex_unlock(&openCardsLock);
	return -1;
    }
    memset(card, 0, sizeof(Open8055_card_t));

    /* ----
     * Initialize the data
     * ----
     */
    card->isLocal = TRUE;
    card->idLocal = cardNumber;
    if (pthread_mutex_init(&(card->lock), NULL) != 0)
    {
    	SetError("pthread_mutex_init() failed - %s", ErrorString());
	free(card);
	openCards[handle] = NULL;
	pthread_mutex_unlock(&openCardsLock);
	return -1;
    }
    if (pthread_cond_init(&(card->cond), NULL) != 0)
    {
    	SetError("pthread_cond_init() failed - %s", ErrorString());
	pthread_mutex_destroy(&(card->lock));
	free(card);
	openCards[handle] = NULL;
	pthread_mutex_unlock(&openCardsLock);
	return -1;
    }

    /* ----
     * Query current card status.
     * ----
     */
    memset(&outputMessage, 0, sizeof(outputMessage));
    outputMessage.msgType = OPEN8055_HID_MESSAGE_GETCONFIG;
    if (DeviceWrite(cardNumber, &outputMessage, sizeof(outputMessage)) < 0)
    {
    	SetError("Sending of GETCONFIG failed - %s", ErrorString);
	pthread_cond_destroy(&(card->cond));
	pthread_mutex_destroy(&(card->lock));
	free(card);
	openCards[handle] = NULL;
	pthread_mutex_unlock(&openCardsLock);
	return -1;
    }
    while (card->currentConfig1.msgType == 0x00 || card->currentOutput.msgType == 0x00
    	|| card->currentInput.msgType == 0x00)
    {
    	if (DeviceRead(cardNumber, &inputMessage, sizeof(inputMessage)) < 0)
	{
	    SetError("DeviceRead failed - %s", ErrorString);
	    pthread_cond_destroy(&(card->cond));
	    pthread_mutex_destroy(&(card->lock));
	    free(card);
	    openCards[handle] = NULL;
	    pthread_mutex_unlock(&openCardsLock);
	    return -1;
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
	    	memcpy(&(card->currentInput), &inputMessage, sizeof(card->currentInput));
		card->currentInputUnconsumed = OPEN8055_INPUT_ANY;
	    	break;
	}
    }

    /* ----
     * Finally launch the IO thread for this card.
     * ----
     */
    if (pthread_create(&(card->ioThread), NULL, CardIOThread, (void *)card) != 0)
    {
    	SetError("pthread_create() failed - %s", ErrorString);
	pthread_cond_destroy(&(card->cond));
	pthread_mutex_destroy(&(card->lock));
	free(card);
	openCards[handle] = NULL;
	pthread_mutex_unlock(&openCardsLock);
	return -1;
    }

    /* ----
     * Success.
     * ----
     */
    pthread_mutex_unlock(&openCardsLock);
    return handle;
}


/* ----
 * Open8055_Close()
 *
 *  Close an Open8055.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_Close(int handle)
{
    Open8055_card_t	*card;
    int			rc = 0;

    OPEN8055_INIT();
    OPEN8055_LOCK_CARD(handle, card);

    /* ----
     * If the ioTerminate flag is set some other thread is already
     * performing a close operation.
     * ----
     */
    if (card->ioTerminate)
    {
    	SetError("Concurrent duplicate Open8055_Close() calls for card %d, handle %d",
			card->idLocal, handle);
	OPEN8055_UNLOCK_CARD(card);
	return -1;
    }

    /* ----
     * Tell the io thread to terminate.
     * ----
     */
    card->ioTerminate = TRUE;
    pthread_mutex_unlock(&(card->lock));
    if (pthread_join(card->ioThread, NULL) != 0)
    {
    	SetError("pthread_join() failed - %s", ErrorString());
	rc = -1;
    }
    DeviceClose(card->idLocal);

    /* ----
     * Before destroying all the other resources, make sure we
     * no longer have any IO waiters.
     * ----
     */
    card->ioError = TRUE;
    strcpy(card->ioErrorMessage, "card is closed");
    while (card->ioWaiters > 0)
    {
	pthread_cond_broadcast(&(card->cond));
	pthread_mutex_unlock(&(card->lock));
	usleep(1000);
	pthread_mutex_lock(&(card->lock));
    }

    /* ----
     * Free the card structure and all its resources. Then mark the
     * slot empty.
     * ----
     */
    pthread_mutex_lock(&openCardsLock);

    pthread_mutex_unlock(&(card->lock));
    pthread_mutex_destroy(&(card->lock));
    pthread_cond_destroy(&(card->cond));
    free(card);
    openCards[handle] = NULL;

    pthread_mutex_unlock(&openCardsLock);

    return rc;
}


/* ----
 * Open8055_Wait()
 *
 *  Wait until any new input becomes available
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_Wait(int handle, long us)
{
    Open8055_card_t	*card;
    int			rc = 0;

    OPEN8055_INIT();
    OPEN8055_LOCK_CARD(handle, card);

    rc = Open8055_WaitForInput(card, OPEN8055_INPUT_ANY, us);

    OPEN8055_UNLOCK_CARD(card);
    return rc;
}


/* ----
 * Open8055_WaitFor()
 *
 *  Wait until specific input ports have changed.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_WaitFor(int handle, uint32_t mask, long us)
{
    Open8055_card_t	*card;
    int			rc = 0;

    OPEN8055_INIT();
    OPEN8055_LOCK_CARD(handle, card);

    rc = Open8055_WaitForInput(card, mask, us);

    OPEN8055_UNLOCK_CARD(card);
    return rc;
}


/* ----
 * Open8055_GetInputDigitalAll()
 *
 *  Read the 5 digital input ports
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_GetInputDigitalAll(int handle)
{
    Open8055_card_t	*card;
    int			rc = 0;

    OPEN8055_INIT();
    OPEN8055_LOCK_CARD(handle, card);

    /* ----
     * Make sure we have current input data.
     * ----
     */
    Open8055_WaitForInput(card, OPEN8055_INPUT_I_ANY, OPEN8055_WAITFOR_US);

    /* ----
     * If the card is in error state, just copy it's error message and 
     * return with error.
     * ----
     */
    if (card->ioError)
    {
    	SetError("%s", card->ioErrorMessage);
	OPEN8055_UNLOCK_CARD(card);
	return -1;
    }

    /* ----
     * Mark all digital inputs as consumed and return the current state.
     * ----
     */
    card->currentInputUnconsumed &= ~OPEN8055_INPUT_I_ANY;
    rc = card->currentInput.inputBits;

    OPEN8055_UNLOCK_CARD(card);
    return rc;
}


/* ----
 * Open8055_GetOutputDigitalAll()
 *
 *  Return the current digital output settings.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_GetOutputDigitalAll(int handle)
{
    Open8055_card_t	*card;
    int			rc = 0;

    OPEN8055_INIT();
    OPEN8055_LOCK_CARD(handle, card);

    /* ----
     * We have queried them at Connect and tracked them all the time.
     * ----
     */
    rc = card->currentOutput.outputBits;

    OPEN8055_UNLOCK_CARD(card);
    return rc;
}



/* ----
 * Open8055_SetOutputDigitalAll()
 *
 *  Schedule the 8 digital outputs to be changed on next IO cycle.
 * ----
 */
OPEN8055_EXTERN int STDCALL
Open8055_SetOutputDigitalAll(int handle, int bits)
{
    Open8055_card_t	*card;
    int			rc = 0;

    OPEN8055_INIT();
    OPEN8055_LOCK_CARD(handle, card);

    /* ----
     * Just set those bits in the next output message and mark that we
     * have some changes to be sent.
     * ----
     */
    card->currentOutput.outputBits = bits;
    if (DeviceWrite(card->idLocal, &(card->currentOutput), sizeof(card->currentOutput)) != 32)
    	rc = -1;

    OPEN8055_UNLOCK_CARD(card);
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

    if (pthread_mutex_init(&openCardsLock, NULL) != 0)
    {
	SetError("pthread_mutex_init() failed - %s", ErrorString());
	return -1;
    }
    if (pthread_mutex_init(&lastErrorLock, NULL) != 0)
    {
	SetError("pthread_mutex_init() failed - %s", ErrorString());
	pthread_mutex_destroy(&openCardsLock);
	return -1;
    }

    if (DeviceInit() < 0)
    {
	pthread_mutex_destroy(&lastErrorLock);
	pthread_mutex_destroy(&openCardsLock);
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
Open8055_WaitForInput(Open8055_card_t *card, uint32_t mask, long us)
{
    int			rc = 0;
    struct timespec	ts;
    struct timeval	now;

    /* ----
     * If input of the requested mask type is available,
     * return immediately.
     * ----
     */
    if ((card->currentInputUnconsumed & mask) != 0)
	return 0;

    /* ----
     * The application consumed this type of input before. Need
     * to wait for another HID report to arrive or until timeout.
     * ----
     */
    card->ioWaiters++;
    if (us > 0)
    {
    	gettimeofday(&now, NULL);
    	ts.tv_sec = now.tv_sec + us / 1000000;
	ts.tv_nsec = (now.tv_usec + (us % 1000000)) * 1000;
	if (ts.tv_nsec >= 1000000000)
	{
	    ts.tv_sec++;
	    ts.tv_nsec -= 1000000000;
	}
	rc = pthread_cond_timedwait(&(card->cond), &(card->lock), &ts);
	if (rc == ETIMEDOUT)
	    rc = 1;
	else if(rc != 0)
	    rc = -1;
    }
    else
    {
        rc = pthread_cond_wait(&(card->cond), &(card->lock));
	if (rc != 0)
	    rc = -1;
    }
    card->ioWaiters--;

    if (rc < 0)
    {
    	SetError("pthread_cond_wait() failed - %s", ErrorString());
	return -1;
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
SetError(char *fmt, ...)
{
    va_list     ap;

    if (initialized)
    	pthread_mutex_lock(&lastErrorLock);

    va_start(ap, fmt);
    vsnprintf(lastErrorMessage, sizeof(lastErrorMessage), fmt, ap);
    va_end(ap);

    if (initialized)
    	pthread_mutex_unlock(&lastErrorLock);
}


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
    int				idLocal;
    int				rc;

    pthread_mutex_lock(&(card->lock));
    idLocal = card->idLocal;

    while(TRUE)
    {
	/* ----
	 * Check for card terminate flag
	 * ----
	 */
	if (card->ioTerminate)
	{
	    OPEN8055_UNLOCK_CARD(card);
	    return NULL;
	}

	/* ----
	 * Wait for a message from the card. Unlock the card status data
	 * so other threads can access it.
	 * ----
	 */
	pthread_mutex_unlock(&(card->lock));
	rc = DeviceRead(idLocal, &inputMessage, sizeof(inputMessage));
	pthread_mutex_lock(&(card->lock));

	/* ----
	 * Check for physical card IO error.
	 * ----
	 */
	if (rc < 0)
	{
	    /* ----
	     * In case of error we copy the current error message as quickly as
	     * possible into the card status structure. We then set the ioError
	     * flag, signal all possible IO waiters and terminate the IO thread.
	     * ----
	     */
	    pthread_mutex_lock(&lastErrorLock);
	    strcpy(card->ioErrorMessage, lastErrorMessage);
	    pthread_mutex_unlock(&lastErrorLock);

	    card->ioError = 1;
	    pthread_cond_broadcast(&(card->cond));
	    pthread_mutex_unlock(&(card->lock));
	    return NULL;
	}

	switch (inputMessage.msgType)
	{
	    case OPEN8055_HID_MESSAGE_INPUT:
		if (memcmp(&(card->currentInput), &inputMessage, sizeof(card->currentInput)) != 0)
		{
		    memcpy(&(card->currentInput), &inputMessage, sizeof(card->currentInput));
		    card->currentInputUnconsumed = OPEN8055_INPUT_ANY;
		    if (card->ioWaiters > 0)
		    {
			pthread_cond_broadcast(&(card->cond));
		    }
		}
		break;

	    case OPEN8055_HID_MESSAGE_SETCONFIG1:
	    case OPEN8055_HID_MESSAGE_OUTPUT:
	    	break;

	    default:
		card->ioError = TRUE;
	    	snprintf(card->ioErrorMessage, sizeof(card->ioErrorMessage),
			"Received unknown message type 0x%02x", inputMessage.msgType);
		pthread_cond_broadcast(&(card->cond));
		pthread_mutex_unlock(&(card->lock));
		return NULL;
	}
    }

    return NULL;
}


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
DeviceOpen(int cardNumber)
{
    char           *path;

    /* ----
     * Lookup the device path for the requested card.
     * ----
     */
    path = DeviceFindPath(cardNumber);
    if (path == NULL)
    {
	SetError("Open8055 card number %d not present", cardNumber);
	return -1;
    }

    /* ----
     * Try to open the device.
     * ----
     */
    cardHandleRecv[cardNumber] = CreateFile(path, GENERIC_READ, 
	    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    cardHandleSend[cardNumber] = CreateFile(path, GENERIC_WRITE, 
	    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    free(path);
    if (cardHandleRecv[cardNumber] == INVALID_HANDLE_VALUE ||
    	cardHandleSend[cardNumber] == INVALID_HANDLE_VALUE)
    {
	SetError("Cannot open Open8055 card number %d - %s", cardNumber, ErrorString());
	free(path);
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
DeviceClose(int cardNumber)
{
    int     rc = 0;

    /* ----
     * Close the handle.
     * ----
     */
    if (!CloseHandle(cardHandleRecv[cardNumber]))
    {
	CloseHandle(cardHandleSend[cardNumber]);
	SetError("Cannot close Open8055 card number %d - %s", cardNumber, ErrorString());
	rc = -1;
    }
    if (!CloseHandle(cardHandleSend[cardNumber]))
    {
	SetError("Cannot close Open8055 card number %d - %s", cardNumber, ErrorString());
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
DeviceRead(int cardNumber, void *buffer, int len)
{
    unsigned char      *ioBuf;
    DWORD       	bytesRead;

    if ((ioBuf = malloc(len + 1)) == NULL)
    {
        SetError("Out of memory in DeviceRead() - failed to allocate %d bytes", len + 1);
	return -1;
    }

    if (!ReadFile(cardHandleRecv[cardNumber], ioBuf, len + 1, &bytesRead, NULL))
    {
	SetError("ReadFile() failed for card %d - %s", cardNumber, ErrorString());
	free(ioBuf);
	return -1;
    }

    if (bytesRead != len + 1)
    {
	SetError("Short read from card %d - expected %d but got %d", cardNumber, len + 1, bytesRead);
	free(ioBuf);
	return -1;
    }

    memcpy(buffer, &ioBuf[1], len);
    free(ioBuf);

    return len;
}


/* ----
 * DeviceWrite()
 *
 *  Send one message to the Open8055.
 * ----
 */
static int
DeviceWrite(int cardNumber, void *buffer, int len)
{
    unsigned char      *ioBuf;
    DWORD       	bytesWritten;

    if ((ioBuf = malloc(len + 1)) == NULL)
    {
        SetError("Out of memory in DeviceWrite() - failed to allocate %d bytes", len + 1);
	return -1;
    }

    ioBuf[0] = '\0';
    memcpy(&ioBuf[1], buffer, len);

    if (!WriteFile(cardHandleSend[cardNumber], ioBuf, len + 1, &bytesWritten, NULL))
    {
	SetError("WriteFile() failed for card %d - %s", cardNumber, ErrorString());
	free(ioBuf);
	return -1;
    }

    free(ioBuf);

    if (bytesWritten != len + 1)
    {
	SetError("Short write to card %d - expected %d but wrote %d", cardNumber, len + 1, bytesWritten);
	return -1;
    }

    return len;
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
	SetError("UuidFromString() failed");
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
	SetError("SetupDiGetClassDevs(): error %ld", GetLastError());
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
	    SetError("malloc(): out of memory");
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
	    SetError("SetupDiGetInterfaceDetail(): error %ld", GetLastError());
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
