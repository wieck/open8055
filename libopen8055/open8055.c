/* ----------------------------------------------------------------------
 * open8055.c
 *
 *  This is an attempt to create a portable library to
 *  access an Open8055 USB Experiment Interface Card.
 *  The code uses libusb 1.0 calls under Unix and native
 *  setupapi calls under Windows.
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

#include <unistd.h>

#include "open8055_compat.h"
#include "open8055.h"
#include "open8055_hid_protocol.h"

#ifndef _WIN32
#include <pthread.h>
#endif

/* ----------------------------------------------------------------------
 * Local definitions
 * ----------------------------------------------------------------------
 */

typedef struct {
    int                     isLocal;
    int                     idLocal;
    char                    destination[1024];

    SOCKET		    sock;
    char		    net_input_buffer[1024];
    char		   *net_input_pos;
    int			    net_input_have;
    char		    net_input_line[1024];
    char		   *net_input_out;

    char                    errorMessage[1024];

    Open8055_hidMessage_t   currentConfig1;
    Open8055_hidMessage_t   currentOutput;
    Open8055_hidMessage_t   currentInput;
    int                     currentInputUnconsumed;

    int                     autoFlush;
    int                     pendingConfig1;
    int                     pendingOutput;
    int                     cardClosed;
    int                     cardRefcount;

#ifdef _WIN32
    unsigned char           writeBuffer[OPEN8055_HID_MESSAGE_SIZE + 1];
    unsigned char           readBuffer[OPEN8055_HID_MESSAGE_SIZE + 1];
    HANDLE                  readEvent;
    int                     readPending;
    OVERLAPPED              readOverlapped;
    CRITICAL_SECTION        cardLock;
#else
    unsigned char           readBuffer[OPEN8055_HID_MESSAGE_SIZE];
    libusb_device_handle    *cardHandle;
    int                     hadKernelDriver;
    struct libusb_transfer  *transfer;
    int                     transferPending;
    int                     transferDone;
    pthread_mutex_t         cardLock;
#endif

} Open8055_card_t;


/* ----------------------------------------------------------------------
 * Local functions
 * ----------------------------------------------------------------------
 */
static Open8055_card_t *LockAndRefcount(int h);
static void UnlockAndRefcount(Open8055_card_t *card);
#ifdef _WIN32
#define LockCreate(_c)      InitializeCriticalSection((_c))
#define LockDestroy(_c)     DeleteCriticalSection((_c))
#define LockAcquire(_c)     EnterCriticalSection((_c))
#define LockRelease(_c)     LeaveCriticalSection((_c))
#else
#define LockCreate(_c)      pthread_mutex_init((_c), NULL)
#define LockDestroy(_c)     pthread_mutex_destroy((_c))
#define LockAcquire(_c)     pthread_mutex_lock((_c))
#define LockRelease(_c)     pthread_mutex_unlock((_c))
#endif

static int Open8055_Init(void);
static void SetError(Open8055_card_t *card, char *fmt, ...);

static int CardRead(Open8055_card_t *card, void *buffer, int timeout);
static int CardReadLine(Open8055_card_t *card, char *buffer, int len, int timeout);
static int CardWrite(Open8055_card_t *card, void *buffer);
static int CardWriteLine(Open8055_card_t *card, char *fmt, ...);
static int CardClose(Open8055_card_t *card);

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
static char             lastErrorMessage[1024] = {'\0'};
static int              initialized = FALSE;

static int              openLocalCards[OPEN8055_MAX_CARDS];

static Open8055_card_t  **connections = NULL;
static int              connectionsSize = 0;
static int              connectionsUsed = 0;
#ifdef _WIN32
static CRITICAL_SECTION connectionsLock;
WSADATA			WSAData;
#else
static pthread_mutex_t  connectionsLock;
#endif


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
OPEN8055_EXTERN char * OPEN8055_CDECL
Open8055_LastError(int h)
{
    Open8055_card_t *card;
    char            *result;

    if (h < 0)
        return lastErrorMessage;

    if ((card = LockAndRefcount(h)) == NULL)
        return lastErrorMessage;

    result = card->errorMessage;

    UnlockAndRefcount(card);
    return result;
}


/* ----
 * Open8055_CardPresent()
 *
 *  Checks if a given card number is present in the local system.
 *  Returns 1 if card is present, 0 if not.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
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
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_Connect(char *destination, char *password)
{
    int                     cardNumber;
    Open8055_card_t        *card;
    Open8055_hidMessage_t   outputMessage;
    Open8055_hidMessage_t   inputMessage;
    int                     handle;

    /* ----
     * Make sure the library is initialized.
     * ----
     */
    if (!initialized)
    {
        if (Open8055_Init() < 0)
            return -1;
    }

    /* ----
     * Allocate the card status data.
     * ----
     */
    card = (Open8055_card_t *)malloc(sizeof(Open8055_card_t));
    if (card == NULL)
    {
        SetError(NULL, "out of memory");
        return -1;
    }
    memset(card, 0, sizeof(Open8055_card_t));
    strncpy(card->destination, destination, sizeof(card->destination));
    card->autoFlush = TRUE;

    /* ----
     * Parse the destination. We first check for the remote
     * format of open8055://user@host/cardN.
     * ----
     */
    if (strncasecmp(destination, "open8055://", 11) == 0)
    {
    	char           *destcopy = strdup(&destination[11]);
	char           *user = "nobody";
	char           *host = "localhost";
	int	        port = 8055;
	char           *parsepos = destcopy;
	char           *pos;
	struct hostent *hent;
	struct sockaddr_in	addr;
	char		line[256];
	char		salt[256];
	int		rc;

	/* ----
	 * If present, extract the USER@ part at the beginning of the destination.
	 * ----
	 */
	if ((pos = strchr(parsepos, '@')) != NULL)
	{
	    user = parsepos;
	    *pos++ = '\0';
	    parsepos = pos;
	}

	/* ----
	 * We now expect either "host:port/cardN" or "host/cardN".
	 * ----
	 */
	if ((pos = strchr(parsepos, ':')) != NULL)
	{
	    /* ----
	     * There is a colon, so get the host and port.
	     * ----
	     */
	    host = parsepos;
	    *pos++ = '\0';
	    parsepos = pos;
	    if ((pos = strchr(parsepos, '/')) == NULL)
	    {
	    	SetError(NULL, "Invalid destination");
		free(card);
		free(destcopy);
		return -1;
	    }
	    *pos++ = '\0';
	    if (sscanf(parsepos, "%d", &port) != 1)
	    {
	    	SetError(NULL, "Invalid destination");
		free(card);
		free(destcopy);
		return -1;
	    }
	    parsepos = pos;
	}
	else
	{
	    /* ----
	     * No colon, so it is just going to be a host name
	     * followed by /cardN.
	     * ----
	     */
	    if ((pos = strchr(parsepos, '/')) == NULL)
	    {
	    	SetError(NULL, "Invalid destination");
		free(card);
		free(destcopy);
		return -1;
	    }
	    *pos++ = '\0';
	    host = parsepos;
	    parsepos = pos;
	}

	/* ----
	 * The final element in the remote card address must be "cardN".
	 * ----
	 */
	if (sscanf(parsepos, "card%d", &cardNumber) != 1)
	{
	    SetError(NULL, "Invalid destination");
	    free(card);
	    free(destcopy);
	    return -1;
	}

	/* ----
	 * Get the actual host address.
	 * ----
	 */
	hent = gethostbyname(host);
	if (hent == NULL)
	{
	    SetError(NULL, "%s: unknown host", host);
	    free(card);
	    free(destcopy);
	    return -1;
	}
	free(destcopy);

	/* ----
	 * Connect to the Open8055Server.
	 * ----
	 */
	addr.sin_family = AF_INET;
	addr.sin_addr = *((struct in_addr *)(hent->h_addr));
	addr.sin_port = htons(port);
	card->sock = socket(AF_INET, SOCK_STREAM, 0);
	if (card->sock == INVALID_SOCKET)
	{
	    SetError(NULL, "%s", ErrorString());
	    free(card);
	    return -1;
	}
	if (connect(card->sock, (struct sockaddr *)&addr, sizeof(addr)) != 0)
	{
	    SetError(NULL, "%s", ErrorString());
	    closesocket(card->sock);
	    free(card);
	    return -1;
	}

	/* ----
	 * Create and acquire the card lock and mark the card being remote.
	 * ----
	 */
	LockCreate(&(card->cardLock));
	LockAcquire(&(card->cardLock));
	card->isLocal   = FALSE;
	card->idLocal   = -1;
	card->net_input_pos = card->net_input_buffer;
	card->net_input_out = card->net_input_line;

	/* ----
	 * Get the HELLO and SALT messages.
	 * ----
	 */
	if ((rc = CardReadLine(card, line, sizeof(line), 60000)) <= 0)
	{
	    if (rc < 0)
		strncpy(lastErrorMessage, card->errorMessage, sizeof(lastErrorMessage));
	    else
	        SetError(NULL, "timeout receiving HELLO");
	    CardClose(card);
	    LockRelease(&(card->cardLock));
	    LockDestroy(&(card->cardLock));
	    free(card);
	    return -1;
	}
	if (strncmp(line, "HELLO Open8055Server ", 21) != 0)
	{
	    SetError(NULL, "Expected HELLO, got '%s'", line);
	    CardClose(card);
	    LockRelease(&(card->cardLock));
	    LockDestroy(&(card->cardLock));
	    free(card);
	    return -1;
	}

	if ((rc = CardReadLine(card, line, sizeof(line), 60000)) <= 0)
	{
	    if (rc < 0)
		strncpy(lastErrorMessage, card->errorMessage, sizeof(lastErrorMessage));
	    else
	        SetError(NULL, "timeout receiving SALT");
	    CardClose(card);
	    LockRelease(&(card->cardLock));
	    LockDestroy(&(card->cardLock));
	    free(card);
	    return -1;
	}
	if (sscanf(line, "SALT %s", salt) != 1)
	{
	    SetError(NULL, "Expected SALT, got '%s'", line);
	    CardClose(card);
	    LockRelease(&(card->cardLock));
	    LockDestroy(&(card->cardLock));
	    free(card);
	    return -1;
	}

	/* ----
	 * Send the OPEN command with username and password.
	 * TODO: MD5 hashing
	 * ----
	 */
    	if (CardWriteLine(card, "open %d %s %s\n", cardNumber, user, "dummy") < 0)
	{
	    strncpy(lastErrorMessage, card->errorMessage, sizeof(lastErrorMessage));
	    CardClose(card);
	    LockRelease(&(card->cardLock));
	    LockDestroy(&(card->cardLock));
	    free(card);
	    return -1;
	}
    }
    else
    {
	/* ----
	 * Destination does not start with "open8055://". The requested card must be a local card.
	 * ----
	 */
	if (sscanf(destination, "card%d", &cardNumber) != 1)
	{
	    SetError(NULL, "Syntax error in local card address '%s'", destination);
	    free(card);
	    return -1;
	}

	/* ----
	 * Check the card number for validity and make sure it isn't open yet.
	 * ----
	 */
	if (cardNumber < 0 || cardNumber >= OPEN8055_MAX_CARDS)
	{
	    SetError(NULL, "Card number %d out of bounds", cardNumber);
	    free(card);
	    return -1;
	}
	if (openLocalCards[cardNumber] != 0)
	{
	    SetError(NULL, "Local card %d already open", cardNumber);
	    free(card);
	    return -1;
	}

	/* ----
	 * Try to open the actual local card.
	 * ----
	 */
	if (DeviceOpen(card) < 0)
	{
	    strncpy(lastErrorMessage, card->errorMessage, sizeof(lastErrorMessage));
	    free(card);
	    return -1;
	}

	/* ----
	 * Create and acquire the card lock.
	 * ----
	 */
	LockCreate(&(card->cardLock));
	LockAcquire(&(card->cardLock));

	card->isLocal   = TRUE;
	card->idLocal   = cardNumber;
    }

    /* ----
     * Query current card status. When connecting to an Open8055Server,
     * The OPEN command automatically responds with those messages.
     * ----
     */
    if (card->isLocal)
    {
	memset(&outputMessage, 0, sizeof(outputMessage));
	outputMessage.msgType = OPEN8055_HID_MESSAGE_GETCONFIG;
	if (CardWrite(card, &outputMessage) < 0)
	{
	    strncpy(lastErrorMessage, card->errorMessage, sizeof(lastErrorMessage));
	    CardClose(card);
	    LockRelease(&(card->cardLock));
	    LockDestroy(&(card->cardLock));
	    free(card);
	    return -1;
	}
    }
    while (card->currentConfig1.msgType == 0x00 
        || card->currentOutput.msgType == 0x00
        || card->currentInput.msgType == 0x00)
    {
        if (CardRead(card, &inputMessage, 1000) < 0)
        {
	    strncpy(lastErrorMessage, card->errorMessage, sizeof(lastErrorMessage));
            CardClose(card);
            LockRelease(&(card->cardLock));
            LockDestroy(&(card->cardLock));
            free(card);
            return -1;
        }
        switch(inputMessage.msgType)
        {
            case OPEN8055_HID_MESSAGE_SETCONFIG1:
                memcpy(&(card->currentConfig1), &inputMessage, 
                    sizeof(card->currentConfig1));
                break;

            case OPEN8055_HID_MESSAGE_OUTPUT:
                memcpy(&(card->currentOutput), &inputMessage, 
                    sizeof(card->currentOutput));
                break;

            case OPEN8055_HID_MESSAGE_INPUT:
                card->currentInputUnconsumed = OPEN8055_INPUT_ANY;
                memcpy(&(card->currentInput), &inputMessage, 
                    sizeof(card->currentInput));
                break;
        }
    }

    /* ----
     * Find a free connection slot or allocate a new one.
     * ----
     */
    for (handle = 0; handle < connectionsUsed; handle++)
    {
        if (connections[handle] == NULL)
            break;
    }
    if (handle == connectionsSize)
    {
        connectionsSize *= 2;
        connections = (Open8055_card_t **)realloc(connections, sizeof(Open8055_card_t *) * connectionsSize);
        if (connections == NULL)
        {
            initialized = 0;
            SetError(NULL, "out of memory");
            CardClose(card);
            LockRelease(&(card->cardLock));
            LockDestroy(&(card->cardLock));
            free(card);
            return -1;
        }
    }
    if (handle == connectionsUsed)
        connectionsUsed++;
    connections[handle] = card;
    LockRelease(&(card->cardLock));

    /* ----
     * Success.
     * ----
     */
    return handle;
}


/* ----
 * Open8055_Close()
 *
 *  Close an Open8055.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_Close(int h)
{
    Open8055_card_t     *card;
    int                 rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    /* ----
     * Guard against concurrent calls of Close/Reset.
     * ----
     */
    if (card->cardClosed)
    {
        UnlockAndRefcount(card);
        return -1;
    }
    strcpy(card->errorMessage, "card closed");
    card->cardClosed = 1;

    /* ----
     * We need both locks, the one of the card as well as the one for the
     * global connections list. To avoid deadlock we must first release
     * the card lock, then acquire the connectionsLock and re-acquire the
     * card lock.
     * ----
     */
    LockRelease(&(card->cardLock));
    LockAcquire(&connectionsLock);
    LockAcquire(&(card->cardLock));

    /* ----
     * We now can safely mark the handle slot empty, so that no other calls
     * for this card can be done. 
     * ----
     */
    connections[h] = NULL;
    LockRelease(&connectionsLock);

    /* ----
     * It is possible that some other call is currently accessing the card.
     * Worst case that could be a blocking WaitForInput(). We request INPUT
     * reports from the card until the reference count drops to one (our
     * own count).
     * ----
     */
    while(card->cardRefcount > 1)
    {
        Open8055_hidMessage_t   message;

        memset(&message, 0, sizeof(message));
        message.msgType = OPEN8055_HID_MESSAGE_GETINPUT;

        if (CardWrite(card, &message) < 0)
        {
            UnlockAndRefcount(card);
            return -1;
        }

        LockRelease(&(card->cardLock));
        usleep(1000);
        LockAcquire(&(card->cardLock));
    }

    /* ----
     * At this point we should be the only one left using this card.
     * Close the device and free the card structure.
     * ----
     */
    if (CardClose(card) < 0)
    {
        strncpy(lastErrorMessage, card->errorMessage, sizeof(lastErrorMessage));
        UnlockAndRefcount(card);
        rc = -1;
    }

    UnlockAndRefcount(card);
    LockDestroy(&(card->cardLock));
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
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_Reset(int h)
{
    Open8055_card_t         *card;
    int                     rc = 0;
    Open8055_hidMessage_t   message;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    /* ----
     * Guard against concurrent calls of Close/Reset.
     * ----
     */
    if (card->cardClosed)
    {
        UnlockAndRefcount(card);
        return -1;
    }
    strcpy(card->errorMessage, "card closed");
    card->cardClosed = 1;

    /* ----
     * We need both locks, the one of the card as well as the one for the
     * global connections list. To avoid deadlock we must first release
     * the card lock, then acquire the connectionsLock and re-acquire the
     * card lock.
     * ----
     */
    LockRelease(&(card->cardLock));
    LockAcquire(&connectionsLock);
    LockAcquire(&(card->cardLock));

    /* ----
     * We now can safely mark the handle slot empty, so that no other calls
     * for this card can be done. 
     * ----
     */
    connections[h] = NULL;
    LockRelease(&connectionsLock);

    /* ----
     * It is possible that some other call is currently accessing the card.
     * Worst case that could be a blocking WaitForInput(). We request INPUT
     * reports from the card until the reference count drops to one (our
     * own count).
     * ----
     */
    while(card->cardRefcount > 1)
    {
        Open8055_hidMessage_t   message;

        memset(&message, 0, sizeof(message));
        message.msgType = OPEN8055_HID_MESSAGE_GETINPUT;

        if (CardWrite(card, &message) < 0)
        {
            UnlockAndRefcount(card);
            return -1;
        }

        LockRelease(&(card->cardLock));
        usleep(1000);
        LockAcquire(&(card->cardLock));
    }

    /* ----
     * Send it the RESET command.
     * ----
     */
    memset(&message, 0, sizeof(message));
    message.msgType = OPEN8055_HID_MESSAGE_RESET;
    if (CardWrite(card, &message) < 0)
    {
        strncpy(lastErrorMessage, card->errorMessage, sizeof(lastErrorMessage));
        UnlockAndRefcount(card);
        rc = -1;
    }

    /* ----
     * Close the card and free all data.
     * ----
     */
    if (CardClose(card) < 0)
    {
        strncpy(lastErrorMessage, card->errorMessage, sizeof(lastErrorMessage));
        UnlockAndRefcount(card);
        rc = -1;
    }

    UnlockAndRefcount(card);
    LockDestroy(&(card->cardLock));
    free(card);

    return rc;
}


/* ----
 * Open8055_Wait()
 *
 *  Wait until any new input becomes available
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_Wait(int h)
{
    int             rc;

    while ((rc = Open8055_WaitEx(h, 1000, 0)) == 0);
    return rc;
}


/* ----
 * Open8055_WaitTimeout()
 *
 *  Wait with timeout until any new input becomes available
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_WaitTimeout(int h, int timeout)
{
    int             rc;

    while ((rc = Open8055_WaitEx(h, timeout, 0)) == 0);
    return rc;
}


/* ----
 * Open8055_WaitEx()
 *
 *  Wait with timeout and skipMessages feature.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_WaitEx(int h, int timeout, int skipMessages)
{
    Open8055_card_t         *card;
    Open8055_hidMessage_t   inputMessage;
    int                     rc = 0;
    int                     haveInput = 0;

    if (timeout < 0)
        timeout = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    /* ----
     * If asked to skip messages, we keep reading with a zero timeout
     * until we get a timeout.
     * ----
     */
    if (skipMessages)
    {
        while (rc == 0)
        {
            memset(&inputMessage, 0, sizeof(inputMessage));
            rc = CardRead(card, &inputMessage, 0);

            if (rc < 0 || card->cardClosed)
            {
                UnlockAndRefcount(card);
                return -1;
            }

            if (rc == 0)
                break;

            /* ----
             * Handle by message type.
             * ----
             */
            switch (inputMessage.msgType)
            {
                case OPEN8055_HID_MESSAGE_INPUT:
                    memcpy(&(card->currentInput), &inputMessage, sizeof(card->currentInput));
                    card->currentInputUnconsumed = OPEN8055_INPUT_ANY;
                    haveInput = 1;
#ifdef _WIN32
                    rc = 0;
#endif
                    break;

                case OPEN8055_HID_MESSAGE_SETCONFIG1:
                case OPEN8055_HID_MESSAGE_OUTPUT:
                    rc = 0;
                    break;

                default:
                    SetError(card, "Received unknown message type 0x%02x (1)", inputMessage.msgType);
                    rc = -1;
            }
        }
    }

    /* ----
     * If we have received anything during the skip phase, we
     * are done. 
     * ----
     */
    if (haveInput)
    {
        UnlockAndRefcount(card);
        return 1;
    }

    /* ----
     * If the timeout is zero, we are done as well. 
     * ----
     */
    if (timeout == 0)
    {
        UnlockAndRefcount(card);
        return 0;
    }

    /* ----
     * Now we really have to wait with timeout.
     * ----
     */
    while (rc == 0 && haveInput == 0)
    {
        memset(&inputMessage, 0, sizeof(inputMessage));
        rc = CardRead(card, &inputMessage, timeout);

        if (rc < 0 || card->cardClosed)
        {
            rc = -1;
            break;
        }

        if (rc == 0)
            return 0;

        /* ----
         * Handle by message type.
         * ----
         */
        switch (inputMessage.msgType)
        {
            case OPEN8055_HID_MESSAGE_INPUT:
                memcpy(&(card->currentInput), &inputMessage, sizeof(card->currentInput));
                card->currentInputUnconsumed = OPEN8055_INPUT_ANY;
                haveInput = 1;
                break;

            case OPEN8055_HID_MESSAGE_SETCONFIG1:
            case OPEN8055_HID_MESSAGE_OUTPUT:
                rc = 0;
                break;

            default:
                SetError(card, "Received unknown message type 0x%02x (2)", inputMessage.msgType);
                rc = -1;
        }
    }

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_GetAutoFlush()
 *
 *  Return the current autoFlush setting.
 * ----
 */
OPEN8055_EXTERN void OPEN8055_CDECL
Open8055_Sleep(int ms)
{
#ifdef _WIN32
    if (ms < 0)
        ms = 0;

    Sleep((DWORD)ms);
#else
    struct timeval  tv;

    if (ms < 0)
        ms = 0;

    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;

    select (0, NULL, NULL, NULL, &tv);
#endif
}


/* ----
 * Open8055_GetAutoFlush()
 *
 *  Return the current autoFlush setting.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_GetAutoFlush(int h)
{
    Open8055_card_t *card;
    int             rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if(card->autoFlush)
        rc = 1;

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_SetAutoFlush()
 *
 *  Set the auto flush feature.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_SetAutoFlush(int h, int flag)
{
    Open8055_card_t *card;
    int             rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    card->autoFlush = (flag != FALSE);

    if (card->autoFlush)
    {
        if (card->pendingConfig1)
        {
            if (CardWrite(card, &(card->currentConfig1)) < 0)
                rc = -1;
            else
                card->pendingConfig1 = FALSE;
        }

        if (rc == 0 && card->pendingOutput)
        {
            if (CardWrite(card, &(card->currentOutput)) < 0)
                rc = -1;
            else
            {
                card->pendingOutput = FALSE;
                card->currentOutput.resetCounter = 0x00;
            }
        }
    }

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_Flush()
 *
 *  Send pending changes to the card.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_Flush(int h)
{
    Open8055_card_t *card;
    int         rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (card->pendingConfig1)
    {
        if (CardWrite(card, &(card->currentConfig1)) < 0)
            rc = -1;
        else
            card->pendingConfig1 = FALSE;
    }

    if (rc == 0 && card->pendingOutput)
    {
        if (CardWrite(card, &(card->currentOutput)) < 0)
            return -1;
        else
        {
            card->pendingOutput = FALSE;
            card->currentOutput.resetCounter = 0x00;
        }
    }

    return rc;
}


/* ----
 * Open8055_GetInput()
 *
 *  Read a digital input port.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_GetInput(int h, int port)
{
    Open8055_card_t *card;
    int         rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 4)
    {
        SetError(card, "parameter invalid");
        UnlockAndRefcount(card);
        return -1;
    }

    /* ----
     * Mark all digital inputs as consumed and return the current state.
     * ----
     */
    rc = (card->currentInput.inputBits & (1 << port)) ? 1 : 0;
    card->currentInputUnconsumed &= ~(OPEN8055_INPUT_I1 << port);

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_GetInputAll()
 *
 *  Read the 5 digital input ports
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_GetInputAll(int h)
{
    Open8055_card_t *card;
    int         rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    /* ----
     * Mark all digital inputs as consumed and return the current state.
     * ----
     */
    card->currentInputUnconsumed &= ~OPEN8055_INPUT_I_ANY;
    rc = card->currentInput.inputBits;

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_GetCounter()
 *
 *  Read the current value of a counter.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_GetCounter(int h, int port)
{
    Open8055_card_t *card;
    int         rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 4)
    {
        SetError(card, "parameter invalid");
        UnlockAndRefcount(card);
        return -1;
    }

    /* ----
     * Mark the counter consumed.
     * ----
     */
    card->currentInputUnconsumed &= ~(OPEN8055_INPUT_COUNT1 << port);
    rc = ntohs(card->currentInput.inputCounter[port]);

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_ResetCounter()
 *
 *  Reset an individual input counter.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_ResetCounter(int h, int port)
{
    Open8055_card_t *card;
    int         rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 4)
    {
        SetError(card, "parameter invalid");
        UnlockAndRefcount(card);
        return -1;
    }

    /* ----
     * Set the value and send it if in autoFlush mode.
     * ----
     */
    card->currentOutput.resetCounter |= (1 << port);
    if (card->autoFlush)
    {
        if (CardWrite(card, &(card->currentOutput)) < 0)
            rc = -1;
        else
        {
            card->pendingOutput = FALSE;
            card->currentOutput.resetCounter = 0x00;
        }
    }
    else
    {
        card->pendingOutput = TRUE;
    }

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_ResetCounterAll()
 *
 *  Reset all input counters.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_ResetCounterAll(int h)
{
    Open8055_card_t *card;
    int         rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    /* ----
     * Set the value and send it if in autoFlush mode.
     * ----
     */
    card->currentOutput.resetCounter |= 0x1F;
    if (card->autoFlush)
    {
        if (CardWrite(card, &(card->currentOutput)) < 0)
            rc = -1;
        else
        {
            card->pendingOutput = FALSE;
            card->currentOutput.resetCounter = 0x00;
        }
    }
    else
    {
        card->pendingOutput = TRUE;
    }

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_GetDebounce()
 *
 *  Set the debounce value of a digital input.
 * ----
 */
OPEN8055_EXTERN double OPEN8055_CDECL
Open8055_GetDebounce(int h, int port)
{
    Open8055_card_t *card;
    double      rc;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 4)
    {
        SetError(card, "parameter invalid");
        UnlockAndRefcount(card);
        return -1.0;
    }

    rc = (double)(ntohs(card->currentConfig1.debounceValue[port]) - 1) / 10.0;

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_SetDebounce()
 *
 *  Set the debounce value of a digital input.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_SetDebounce(int h, int port, double ms)
{
    Open8055_card_t *card;
    int         rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 4)
    {
        SetError(card, "parameter invalid");
        UnlockAndRefcount(card);
        return -1;
    }

    if (ms < 0.0)
        ms = 0.0;
    if (ms > 5000.0)
        ms = 5000.0;

    /* ----
     * Set the value and send it if in autoFlush mode.
     * ----
     */
    card->currentConfig1.debounceValue[port] = htons((uint16_t)floor(ms * 10.0) + 1);
    if (card->autoFlush)
    {
        if (CardWrite(card, &(card->currentConfig1)) < 0)
            rc = -1;
        else
            card->pendingConfig1 = FALSE;
    }
    else
    {
        card->pendingConfig1 = TRUE;
    }

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_GetADC()
 *
 *  Read the current value of an ADC
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_GetADC(int h, int port)
{
    Open8055_card_t *card;
    int         rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 1)
    {
        SetError(card, "parameter error");
        UnlockAndRefcount(card);
        return -1;
    }

    /* ----
     * Mark the counter consumed.
     * ----
     */
    card->currentInputUnconsumed &= ~(OPEN8055_INPUT_ADC1 << port);
    rc = ntohs(card->currentInput.inputAdcValue[port]);
    switch(card->currentConfig1.modeADC[port])
    {
        case OPEN8055_MODE_ADC9:        rc >>= 1;
                                        break;
        case OPEN8055_MODE_ADC8:        rc >>= 2;
                                        break;
    }

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_GetOutput()
 *
 *  Return one current digital output settings.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_GetOutput(int h, int port)
{
    Open8055_card_t *card;
    int         rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 7)
    {
        SetError(card, "parameter invalid");
        UnlockAndRefcount(card);
        return -1;
    }

    /* ----
     * We have queried them at Connect and tracked them all the time.
     * ----
     */
    rc = (card->currentOutput.outputBits & (1 << port)) ? 1 : 0;

    UnlockAndRefcount(card);
    return rc;
}



/* ----
 * Open8055_GetOutputAll()
 *
 *  Return the current digital output settings.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_GetOutputAll(int h)
{
    Open8055_card_t *card;
    int         rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    /* ----
     * We have queried them at Connect and tracked them all the time.
     * ----
     */
    rc = card->currentOutput.outputBits;

    UnlockAndRefcount(card);
    return rc;
}



/* ----
 * Open8055_GetOutputValue()
 *
 *  Return the current digital output value used for modes like SERVO.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_GetOutputValue(int h, int port)
{
    Open8055_card_t *card;
    int         rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 7)
    {
        SetError(card, "parameter invalid");
        UnlockAndRefcount(card);
        return -1;
    }

    /* ----
     * We have queried them at Connect and tracked them all the time.
     * ----
     */
    rc = ntohs(card->currentOutput.outputValue[port]);

    UnlockAndRefcount(card);
    return rc;
}



/* ----
 * Open8055_GetPWM()
 *
 *  Return the current setting of a PWM output
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_GetPWM(int h, int port)
{
    Open8055_card_t *card;
    int         rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 1)
    {
        SetError(card, "parameter invalid");
        UnlockAndRefcount(card);
        return -1;
    }

    /* ----
     * We have queried them at Connect and tracked them all the time.
     * ----
     */
    rc = ntohs(card->currentOutput.outputPwmValue[port]);

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_SetOutput()
 *
 *  Change one digital output.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_SetOutput(int h, int port, int val)
{
    Open8055_card_t *card;
    int         rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 7)
    {
        SetError(card, "parameter invalid");
        UnlockAndRefcount(card);
        return -1;
    }

    /* ----
     * Set or clear the bit and send the new info if in autoFlush mode.
     * ----
     */
    if (val)
        card->currentOutput.outputBits |= (1 << port);
    else
        card->currentOutput.outputBits &= ~(1 << port);

    if (card->autoFlush)
    {
        if (CardWrite(card, &(card->currentOutput)) < 0)
            rc = -1;
        else
        {
            card->pendingOutput = FALSE;
            card->currentOutput.resetCounter = 0x00;
        }
    }
    else
    {
        card->pendingOutput = TRUE;
    }

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_SetOutputAll()
 *
 *  Change all 8 digital outputs.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_SetOutputAll(int h, int bits)
{
    Open8055_card_t *card;
    int         rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    bits &= 0xff;

    /* ----
     * Set the bits and send them if in autoFlush mode.
     * ----
     */
    card->currentOutput.outputBits = bits;
    if (card->autoFlush)
    {
        if (CardWrite(card, &(card->currentOutput)) < 0)
            rc = -1;
        else
        {
            card->pendingOutput = FALSE;
            card->currentOutput.resetCounter = 0x00;
        }
    }
    else
    {
        card->pendingOutput = TRUE;
    }

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_SetOutputValue()
 *
 *  Change one digital output value used in modes like SERVO.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_SetOutputValue(int h, int port, int val)
{
    Open8055_card_t *card;
    int         rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 7)
    {
        SetError(card, "parameter invalid");
        UnlockAndRefcount(card);
        return -1;
    }

    switch(card->currentConfig1.modeOutput[port])
    {
        case OPEN8055_MODE_OUTPUT:      // value makes no sense in OUTPUT mode.
            val = 0;
            break;

        case OPEN8055_MODE_SERVO:       // limit value to 500..2500ms in SERVO modes.
        case OPEN8055_MODE_ISERVO:
            if (val < 6000)
                val = 6000;
            if (val > 30000)
                val = 30000;
            break;

        default:
            val = 0;
            break;
    }

    /* ----
     * Set the value in currentOutput and send the new info if in autoFlush mode.
     * ----
     */
    card->currentOutput.outputValue[port] = htons(val);

    if (card->autoFlush)
    {
        if (CardWrite(card, &(card->currentOutput)) < 0)
            rc = -1;
        else
        {
            card->pendingOutput = FALSE;
            card->currentOutput.resetCounter = 0x00;
        }
    }
    else
    {
        card->pendingOutput = TRUE;
    }

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_SetPWM()
 *
 *  Change one of the PWM outputs.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_SetPWM(int h, int port, int value)
{
    Open8055_card_t *card;
    int         rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 1)
    {
        SetError(card, "parameter invalid");
        UnlockAndRefcount(card);
        return 0;
    }

    if (value < 0)
        value = 0;
    if (value > 1023)
        value = 1023;

    /* ----
     * Set the new PWM value and send it if in autoFlush mode.
     * ----
     */
    card->currentOutput.outputPwmValue[port] = htons(value);
    if (card->autoFlush)
    {
        if (CardWrite(card, &(card->currentOutput)) < 0)
            rc = -1;
        else
        {
            card->pendingOutput = FALSE;
            card->currentOutput.resetCounter = 0x00;
        }
    }
    else
    {
        card->pendingOutput = TRUE;
    }

    UnlockAndRefcount(card);
    return rc;
}



/* ----
 * Open8055_GetModeADC()
 *
 *  Return the operation mode of an analog input.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_GetModeADC(int h, int port)
{
    Open8055_card_t *card;
    int             rc;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 1)
    {
        SetError(card, "parameter invalid");
        UnlockAndRefcount(card);
        return -1;
    }

    rc = card->currentConfig1.modeADC[port];

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_SetModeADC()
 *
 *  Set the operation mode of an analog input.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_SetModeADC(int h, int port, int mode)
{
    Open8055_card_t *card;
    int             rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 1)
    {
        SetError(card, "parameter error");
        UnlockAndRefcount(card);
        return -1;
    }

    if (mode == OPEN8055_MODE_ADC10 || mode == OPEN8055_MODE_ADC9 || mode == OPEN8055_MODE_ADC8)
    {
        card->currentConfig1.modeADC[port] = mode;
        if (card->autoFlush)
        {
            if (CardWrite(card, &(card->currentConfig1)) < 0)
                rc = -1;
            else
                card->pendingConfig1 = FALSE;
        }
        else
        {
            card->pendingConfig1 = TRUE;
        }
    }

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_GetModeInput()
 *
 *  Return the operation mode of a digital input.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_GetModeInput(int h, int port)
{
    Open8055_card_t *card;
    int             rc;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 4)
    {
        SetError(card, "parameter invalid");
        UnlockAndRefcount(card);
        return -1;
    }

    rc = card->currentConfig1.modeInput[port];

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_SetModeInput()
 *
 *  Set the operation mode of a digital input.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_SetModeInput(int h, int port, int mode)
{
    Open8055_card_t *card;
    int             rc = 0;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 4)
    {
        SetError(card, "parameter error");
        UnlockAndRefcount(card);
        return -1;
    }

    if (mode == OPEN8055_MODE_INPUT || mode == OPEN8055_MODE_FREQUENCY)
    {
        card->currentConfig1.modeInput[port] = mode;
        if (card->autoFlush)
        {
            if (CardWrite(card, &(card->currentConfig1)) < 0)
                rc = -1;
            else
                card->pendingConfig1 = FALSE;
        }
        else
        {
            card->pendingConfig1 = TRUE;
        }
        /* ----
         * Changing input mode also causes a counter reset.
         * ----
         */
        card->currentOutput.resetCounter |= (1 << port);
        if (card->autoFlush)
        {
            if (CardWrite(card, &(card->currentOutput)) < 0)
                rc = -1;
            else
            {
                card->pendingOutput = FALSE;
                card->currentOutput.resetCounter = 0x00;
            }
        }
        else
        {
            card->pendingOutput = TRUE;
        }
    }

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_GetModeOutput()
 *
 *  Return the operation mode of a digital output.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_GetModeOutput(int h, int port)
{
    Open8055_card_t *card;
    int             rc;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 7)
    {
        SetError(card, "parameter invalid");
        UnlockAndRefcount(card);
        return -1;
    }

    rc = card->currentConfig1.modeOutput[port];

    UnlockAndRefcount(card);
    return rc;
}


/* ----
 * Open8055_SetModeOutput()
 *
 *  Set the operation mode of a digital output.
 * ----
 */
OPEN8055_EXTERN int OPEN8055_CDECL
Open8055_SetModeOutput(int h, int port, int mode)
{
    Open8055_card_t *card;
    int             rc = 0;
    int             val;

    if ((card = LockAndRefcount(h)) == NULL)
        return -1;

    if (port < 0 || port > 7)
    {
        SetError(card, "parameter invalid");
        UnlockAndRefcount(card);
        return -1;
    }

    val = ntohs(card->currentOutput.outputValue[port]);
    switch (mode)
    {
        case OPEN8055_MODE_OUTPUT:
            val = 0;
            break;

        case OPEN8055_MODE_SERVO:
        case OPEN8055_MODE_ISERVO:
            /* ----
             * When we enter SERVO mode, we set the pulse width to center.
             * Otherwise we constrain the pulse width to 0.5 .. 2.5 ms.
             * ----
             */
            if (card->currentConfig1.modeOutput[port] != OPEN8055_MODE_SERVO &&
                card->currentConfig1.modeOutput[port] != OPEN8055_MODE_ISERVO)
            {
                val = 18000;
            }
            else
            {
                if (val < 6000)
                    val = 6000;
                if (val > 30000)
                    val = 30000;
            }
            break;

        default:
            val = 0;
            break;
    }
    val = htons(val);

    if (mode == OPEN8055_MODE_OUTPUT || mode == OPEN8055_MODE_SERVO || mode == OPEN8055_MODE_ISERVO)
    {
        card->currentConfig1.modeOutput[port] = mode;
        if (card->autoFlush)
        {
            if (CardWrite(card, &(card->currentConfig1)) < 0)
                rc = -1;
            else
                card->pendingConfig1 = FALSE;
        }
        else
        {
            card->pendingConfig1 = TRUE;
        }

        if (val != card->currentOutput.outputValue[port])
        {
            card->currentOutput.outputValue[port] = val;
            if (card->autoFlush)
            {
                if (CardWrite(card, &(card->currentOutput)) < 0)
                    rc = -1;
                else
                    card->pendingOutput = FALSE;
            }
            else
            {
                card->pendingOutput = TRUE;
            }
        }
    }

    UnlockAndRefcount(card);
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

    LockCreate(&connectionsLock);

    connectionsSize = 16;
    connections = (Open8055_card_t **)malloc(sizeof(Open8055_card_t *) * 16);
    if (connections == NULL)
    {
        SetError(NULL, "out of memory");
        return -1;
    }

    if (DeviceInit() < 0)
        return -1;

#ifdef _WIN32
    if (WSAStartup(MAKEWORD(2,2), &WSAData) != 0)
    {
    	SetError(NULL, "WSAStartup() failed");
	return -1;
    }
#endif

    initialized = TRUE;
    return 0;
}


static Open8055_card_t *
LockAndRefcount(int h)
{
    Open8055_card_t     *card;

    if (!initialized)
    {
        if (Open8055_Init() < 0)
            return NULL;
    }

    LockAcquire(&connectionsLock);

    if (h < 0 || h >= connectionsUsed || connections[h] == NULL)
    {
        SetError(NULL, "invalid card handle %d", h);
        LockRelease(&connectionsLock);
        return NULL;
    }

    card = connections[h];
    LockAcquire(&(card->cardLock));
    LockRelease(&connectionsLock);
    card->cardRefcount++;

    return card;
}


static void
UnlockAndRefcount(Open8055_card_t *card)
{
    card->cardRefcount--;
    LockRelease(&(card->cardLock));
    return;
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
        vsnprintf(lastErrorMessage, sizeof(lastErrorMessage), fmt, ap);
    else
        vsnprintf(card->errorMessage, sizeof(card->errorMessage), fmt, ap);
    va_end(ap);
}


/* ----
 * CardRead()
 *
 *  Read one HID report from an open card. In case of a local card, DeviceRead()
 *  is used. In the case of a remote card, this function handles the TCP/IP
 *  communication with the server.
 * ----
 */
static int
CardRead(Open8055_card_t *card, void *buffer, int timeout)
{
    char	line[256];
    int		rc;
    int		msgType;
    int		values[24];
    Open8055_hidMessage_t *message;

    if (card->isLocal)
    	return DeviceRead(card, buffer, timeout);

    if ((rc = CardReadLine(card, line, sizeof(line), timeout)) <= 0)
	return rc;

    if (sscanf(line, "RECV %d ", &msgType) != 1)
    {
	if (strncmp(line, "ERROR ", 6) == 0)
	    SetError(card, "%s", line);
	else
	    SetError(card, "Expected RECV - got '%s'", line);
	return -1;
    }

    memset(buffer, 0, OPEN8055_HID_MESSAGE_SIZE);
    message = (Open8055_hidMessage_t *)buffer;

    switch (msgType)
    {
	case OPEN8055_HID_MESSAGE_INPUT:
		if (sscanf(line, "RECV %d %d %d %d %d %d %d %d %d",
			&values[0], &values[1], &values[2], &values[3],
			&values[4], &values[5], &values[6], &values[7],
			&values[8]) != 9)
		{
		    SetError(card, "CardRead(): incomplete INPUT message");
		    return -1;
		}
		message->msgType = values[0];
		message->inputBits = values[1];
		message->inputCounter[0] = ntohs(values[2]);
		message->inputCounter[1] = ntohs(values[3]);
		message->inputCounter[2] = ntohs(values[4]);
		message->inputCounter[3] = ntohs(values[5]);
		message->inputCounter[4] = ntohs(values[6]);
		message->inputAdcValue[0] = ntohs(values[7]);
		message->inputAdcValue[1] = ntohs(values[8]);
		return 1;

	case OPEN8055_HID_MESSAGE_OUTPUT:
		if (sscanf(line, "RECV %d %d %d %d %d %d %d %d %d %d %d %d %d",
			&values[0], &values[1], &values[2], &values[3],
			&values[4], &values[5], &values[6], &values[7],
			&values[8], &values[9], &values[10], &values[11],
			&values[12]) != 13)
		{
		    SetError(card, "CardRead(): incomplete INPUT message");
		    return -1;
		}
		message->msgType = values[0];
		message->outputBits = values[1];
		message->outputValue[0] = ntohs(values[2]);
		message->outputValue[1] = ntohs(values[3]);
		message->outputValue[2] = ntohs(values[4]);
		message->outputValue[3] = ntohs(values[5]);
		message->outputValue[4] = ntohs(values[6]);
		message->outputValue[5] = ntohs(values[7]);
		message->outputValue[6] = ntohs(values[8]);
		message->outputValue[7] = ntohs(values[9]);
		message->outputPwmValue[0] = ntohs(values[10]);
		message->outputPwmValue[1] = ntohs(values[11]);
		message->resetCounter = values[12];
		return 1;

	case OPEN8055_HID_MESSAGE_SETCONFIG1:
		if (sscanf(line, "RECV %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
			&values[0], &values[1], &values[2], &values[3],
			&values[4], &values[5], &values[6], &values[7],
			&values[8], &values[9], &values[10], &values[11],
			&values[12], &values[13], &values[14], &values[15],
			&values[16], &values[17], &values[18], &values[19],
			&values[20], &values[21], &values[22], &values[23]) != 24)
		{
		    SetError(card, "CardRead(): incomplete SETCONFIG1 message");
		    return -1;
		}
		message->msgType = values[0];
		message->modeADC[0] = values[1];
		message->modeADC[1] = values[2];
		message->modeInput[0] = values[3];
		message->modeInput[1] = values[4];
		message->modeInput[2] = values[5];
		message->modeInput[3] = values[6];
		message->modeInput[4] = values[7];
		message->modeOutput[0] = values[8];
		message->modeOutput[1] = values[9];
		message->modeOutput[2] = values[10];
		message->modeOutput[3] = values[11];
		message->modeOutput[4] = values[12];
		message->modeOutput[5] = values[13];
		message->modeOutput[6] = values[14];
		message->modeOutput[7] = values[15];
		message->modePWM[0] = values[16];
		message->modePWM[1] = values[17];
		message->debounceValue[0] = ntohs(values[18]);
		message->debounceValue[1] = ntohs(values[19]);
		message->debounceValue[2] = ntohs(values[20]);
		message->debounceValue[3] = ntohs(values[21]);
		message->debounceValue[4] = ntohs(values[22]);
		message->cardAddress = values[23];
		return 1;

    	default:
		// SetError(card, "CardRead(): unknown message type 0x%02x", msgType);
		SetError(card, "CardRead(): line='%s'", line);
		return -1;
    }
}


/* ----
 * CardReadLine()
 *
 *  Helper function for CardRead() to receive one single line at a time
 *  with IO buffering.
 * ----
 */
static int
CardReadLine(Open8055_card_t *card, char *buffer, int buflen, int timeout)
{
    fd_set		rfds;
    struct timeval	tv;
    int			rc;

    /* ----
     * Don't allow the caller to request more than what we can handle with
     * the cards line buffering.
     * ----
     */
    if (buflen > sizeof(card->net_input_line))
    	buflen = sizeof(card->net_input_line);

    /* ----
     * Receive one line from the server.
     * ----
     */
    for (;;)
    {
	while (card->net_input_have > 0)
	{
	    /* ----
	     * Discard any carriage return characters.
	     * ----
	     */
	    if (*(card->net_input_pos) == '\r')
	    {
		card->net_input_pos++;
		card->net_input_have--;
		continue;
	    }

	    /* ----
	     * If we found a line feed, we got a complete line. Terminate
	     * the line buffer, copy the content to the user buffer and
	     * reset it.
	     * ----
	     */
	    if (*(card->net_input_pos) == '\n')
	    {
		card->net_input_pos++;
		card->net_input_have--;
		*(card->net_input_out) = '\0';
		strncpy(buffer, card->net_input_line, buflen);

		card->net_input_out = card->net_input_line;
		return 1;
	    }

	    /* ----
	     * Ordinary character. Just copy it into the line buffer
	     * and keep moving, unless we run out of space.
	     * ----
	     */
	    *(card->net_input_out++) = *(card->net_input_pos++);
	    card->net_input_have--;

	    if (card->net_input_out >= (card->net_input_line + buflen))
	    {
	    	SetError(card, "Server sent oversize line");
		return -1;
	    }
    	}

	/* ----
	 * We ran out of data from the server. Check if there is more
	 * according to our timeout requirements.
	 * ----
	 */
	if (timeout < 0)
	    timeout = 0;
	FD_ZERO(&rfds);
	FD_SET(card->sock, &rfds);
	tv.tv_sec  = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;
	LockRelease(&(card->cardLock));
	rc = select(card->sock + 1, &rfds, NULL, NULL, &tv);
	LockAcquire(&(card->cardLock));
	if (rc < 0)
	{
	    SetError(card, "select(): %s", ErrorString());
	    return -1;
	}
	if (rc == 0)
	    return 0;

	/* ----
	 * More data is available. Receive it.
	 * ----
	 */
	rc = recv(card->sock, card->net_input_buffer, sizeof(card->net_input_buffer), 0);
	if (rc < 0)
	{
	    SetError(card, "%s", ErrorString());
	    return -1;
	}
	if (rc == 0)
	{
	    SetError(card, "Server closed connection");
	    return -1;
	}
	card->net_input_have = rc;
	card->net_input_pos = card->net_input_buffer;
    }
}


/* ----
 * CardWrite()
 *
 *  Write an HID command message to the card. For local cards use DeviceWrite().
 *  For remote cards translate it into the SEND command and send it to the server.
 * ----
 */
static int
CardWrite(Open8055_card_t *card, void *buffer)
{
    Open8055_hidMessage_t  *message;

    if (card->isLocal)
    	return DeviceWrite(card, buffer);

    message = (Open8055_hidMessage_t *)buffer;
    switch (message->msgType)
    {
	case OPEN8055_HID_MESSAGE_OUTPUT:
		return CardWriteLine(card, "SEND %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
			message->msgType, message->outputBits,
			htons(message->outputValue[0]), htons(message->outputValue[1]),
			htons(message->outputValue[2]), htons(message->outputValue[3]),
			htons(message->outputValue[4]), htons(message->outputValue[5]),
			htons(message->outputValue[6]), htons(message->outputValue[7]),
			htons(message->outputPwmValue[0]), htons(message->outputPwmValue[1]),
			message->resetCounter);

	case OPEN8055_HID_MESSAGE_SETCONFIG1:
		return CardWriteLine(card, "SEND %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
			message->msgType,
			message->modeADC[0], message->modeADC[1],
			message->modeInput[0], message->modeInput[1], message->modeInput[2],
			message->modeInput[3], message->modeInput[4],
			message->modeOutput[0], message->modeOutput[1],
			message->modeOutput[2], message->modeOutput[3],
			message->modeOutput[4], message->modeOutput[5],
			message->modeOutput[6], message->modeOutput[7],
			message->modePWM[0], message->modePWM[1],
			htons(message->debounceValue[0]), htons(message->debounceValue[1]),
			htons(message->debounceValue[1]), htons(message->debounceValue[3]),
			htons(message->debounceValue[4]),
			message->cardAddress);

	case OPEN8055_HID_MESSAGE_GETINPUT:
	case OPEN8055_HID_MESSAGE_GETCONFIG:
	case OPEN8055_HID_MESSAGE_SAVECONFIG:
	case OPEN8055_HID_MESSAGE_SAVEALL:
	case OPEN8055_HID_MESSAGE_RESET:
		return CardWriteLine(card, "SEND %d\n", message->msgType);

    	default:	
		SetError(card, "CardWrite(): unknown message type 0x%02x", message->msgType);
		return -1;

    }

    return 0;
}


/* ----
 * CardWriteLine()
 *
 *  Helper function for CardWrite() to format and send a server message.
 * ----
 */
static int
CardWriteLine(Open8055_card_t *card, char *fmt, ...)
{
    char	buf[256];
    va_list     ap;

    if (card->sock == INVALID_SOCKET)
    {
    	SetError(card, "CardWriteLine(): card is closed");
	return -1;
    }

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (send(card->sock, buf, strlen(buf), 0) != strlen(buf))
    {
    	SetError(card, "send(): %s", ErrorString());
	return -1;
    }

    return 0;
}


/* ----
 * CardClose()
 *
 *  Local/Remote card abstraction for close.
 * ----
 */
static int
CardClose(Open8055_card_t *card)
{
    if (card->isLocal)
    	return DeviceClose(card);

    if (card->sock != INVALID_SOCKET)
    {
	send(card->sock, "quit\n", 5, 0);
	closesocket(card->sock);
	card->sock = INVALID_SOCKET;
	return 0;
    }
    else
    {
    	SetError(card, "Card already closed");
	return -1;
    }
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
     * Check if this device exists.
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
    int                 rc = 0;
    Open8055_hidMessage_t   message;

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
    DWORD               bytesRead;

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
        int     rc;

        LockRelease(&(card->cardLock));
        rc = WaitForSingleObject(card->readEvent, timeout);
        LockAcquire(&(card->cardLock));
        switch(rc)
        {
            case WAIT_OBJECT_0:
                ResetEvent(card->readEvent);
                break;

            case WAIT_TIMEOUT:
                return 0;

            default:
                return -1;
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
    DWORD           bytesWritten;

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
    sprintf(vidPidString, "vid_10cf&pid_%04x", OPEN8055_PID + cardNumber);

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
        SetError(NULL, "libusb_init() failed - %s", ErrorString());
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
    struct libusb_device            **deviceList;
    struct libusb_device_descriptor deviceDesc;
    int                         i;

    /* ----
     * Get the list of USB devices in the system.
     * ----
     */
    numDevices = libusb_get_device_list(libusbCxt, &deviceList);
    if (numDevices < 0)
    {
        SetError(NULL, "libusb_get_device_list(): %s", ErrorString());
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
        SetError(NULL, "libusb_get_device_descriptor(): %s", ErrorString());
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
DeviceOpen(Open8055_card_t *card)
{
    libusb_device_handle   *dev;
    int                     rc;
    int                     interface = 0;

    /* ----
     * Open the device.
     * ----
     */
    dev = card->cardHandle = libusb_open_device_with_vid_pid(
            libusbCxt, OPEN8055_VID, OPEN8055_PID + card->idLocal);
    if (dev == NULL)
    {
        SetError(card, "libusb_open_device_with_vid_pid(): %s", ErrorString());
        return -1;
    }

    /* ----
     * If a kernel driver is active, detach it.
     * ----
     */
    if ((rc = libusb_kernel_driver_active(dev, interface)) < 0)
    {
        SetError(card, "libusb_kernel_driver_active(): %s", ErrorString());
        libusb_close(dev);
        return -1;
    }
    else
    {
        card->hadKernelDriver = rc;
        if (rc != 0)
        {
            if (libusb_detach_kernel_driver(dev, interface) != 0)
            {
                SetError(card, "libusb_detach_kernel_driver(): %s", ErrorString());
                libusb_close(dev);
                return -1;
            }
        }
    }

    /* ----
     * Set configuration
     * ----
     */
    if (libusb_set_configuration(dev, 1) != 0)
    {
        SetError(card, "libusb_set_configuration(): %s", ErrorString());
        libusb_close(dev);
        return -1;
    }

    /* ----
     * Claim the interface.
     * ----
     */
    if (libusb_claim_interface(dev, interface) != 0)
    {
        SetError(card, "libusb_claim_interface(): %s", ErrorString());
        if (card->hadKernelDriver)
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
        SetError(card, "libusb_set_interface_alt_setting(): %s", ErrorString());
        libusb_release_interface(dev, interface);
        if (card->hadKernelDriver)
            libusb_attach_kernel_driver(dev, interface);
        libusb_close(dev);
        return -1;
    }

    /* ----
     * Allocate the libusb_transfer structure for async IO.
     * ----
     */
    if ((card->transfer = libusb_alloc_transfer(0)) == NULL)
    {
        SetError(card, "libusb_alloc_transfer(): %s", ErrorString());
        libusb_release_interface(dev, interface);
        if (card->hadKernelDriver)
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
DeviceClose(Open8055_card_t *card)
{
    int             interface = 0;
    struct timeval  tv;

    /* ----
     * If an IO is pending, cancel it and wait for the callback
     * to have happened.
     * ----
     */
    if (card->transferPending)
        libusb_cancel_transfer(card->transfer);
    
    while (card->transferPending)
    {
        tv.tv_sec = 0;
        tv.tv_usec = 1000;
        libusb_handle_events_timeout(libusbCxt, &tv);
    }

    /* ----
     * Free all resources and close the device.
     * ----
     */
    libusb_free_transfer(card->transfer);
    libusb_release_interface(card->cardHandle, interface);
    if (card->hadKernelDriver)
        libusb_attach_kernel_driver(card->cardHandle, interface);
    libusb_close(card->cardHandle);

    return 0;
}


/* ----
 * DeviceReadCallback()
 *
 *  Libusb callback for async transfer complete.
 * ----
 */
static void
DeviceReadCallback(struct libusb_transfer *transfer)
{
    Open8055_card_t     *card = (Open8055_card_t *)(transfer->user_data);

    card->transferPending = FALSE;
    card->transferDone = TRUE;
}


/* ----
 * DeviceRead()
 *
 *  Receive one message from the Open8055.
 * ----
 */
static int
DeviceRead(Open8055_card_t *card, void *buffer, int timeout)
{
    struct timeval  tv;
    int             rc;

    /* ----
     * transferDone is TRUE if a previously submitted transfer
     * ended. Reset the flag and check the completion status.
     * ----
     */
    if (card->transferDone)
    {
        card->transferDone = FALSE;
        if (card->transfer->status != LIBUSB_TRANSFER_COMPLETED)
        {
            SetError(card, "libusb transfer failed: %s", ErrorString());
            return -1;
        }

        /* ----
         * We have received a new report. Copy it to the caller.
         * ----
         */
        memcpy(buffer, card->readBuffer, OPEN8055_HID_MESSAGE_SIZE);

        /* ----
         * Submit the next transfer right away and call the event
         * handling with a short timeout so that we keep the input
         * buffers inside the USB stack empty.
         * ----
         */
        libusb_fill_interrupt_transfer(card->transfer, card->cardHandle,
                LIBUSB_ENDPOINT_IN | 1, card->readBuffer, 
                OPEN8055_HID_MESSAGE_SIZE,
                DeviceReadCallback, (void *)card, 0);
        if (libusb_submit_transfer(card->transfer) != 0)
        {
            SetError(card, "libusb_submit_transfer(): %s", ErrorString());
            return -1;
        }
        card->transferPending = TRUE;

        tv.tv_sec = 0;
        tv.tv_usec = 100;
        LockRelease(&(card->cardLock));
        rc = libusb_handle_events_timeout(libusbCxt, &tv);
        LockAcquire(&(card->cardLock));
        if (rc != 0)
        {
            SetError(card, "libusb_handle_events_timeout(): %s",
                    ErrorString());
            return -1;
        }

        return 1;
    }

    /* ----
     * Make sure the timeout is sane.
     * ----
     */
    if (timeout < 0)
        timeout = 0;
    tv.tv_sec  = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    /* ----
     * If there is no transfer pending, submit one.
     * ----
     */
    if (!card->transferPending)
    {
        libusb_fill_interrupt_transfer(card->transfer, card->cardHandle,
                LIBUSB_ENDPOINT_IN | 1, card->readBuffer, 
                OPEN8055_HID_MESSAGE_SIZE,
                DeviceReadCallback, (void *)card, 0);
        if (libusb_submit_transfer(card->transfer) != 0)
        {
            SetError(card, "libusb_submit_transfer(): %s", ErrorString());
            return -1;
        }
        card->transferPending = TRUE;

        /* ----
         * Since we just submitted the transfer, make sure the event
         * handling below has at least 100us to interact with the card.
         * ----
         */
        if (timeout == 0)
            tv.tv_usec = 100;
    }

    /* ----
     * Call the libusb event handling with the requested timeout.
     * ----
     */
    LockRelease(&(card->cardLock));
    rc = libusb_handle_events_timeout(libusbCxt, &tv);
    LockAcquire(&(card->cardLock));
    if (rc != 0)
    {
        SetError(card, "libusb_handle_events_timeout(): %s",
                ErrorString());
        return -1;
    }

    /* ----
     * If the transfer is still pending we have a timeout.
     * ----
     */
    if (card->transferPending)
        return 0;

    /* ----
     * Reset the transfer done flag and check the completion status.
     * ----
     */
    card->transferDone = FALSE;
    if (card->transfer->status == LIBUSB_TRANSFER_COMPLETED)
    {
        /* ----
         * We have a new report. Like above, copy to caller
         * and submit the next transfer.
         * ----
         */
        memcpy(buffer, card->readBuffer, OPEN8055_HID_MESSAGE_SIZE);

        libusb_fill_interrupt_transfer(card->transfer, card->cardHandle,
                LIBUSB_ENDPOINT_IN | 1, card->readBuffer, 
                OPEN8055_HID_MESSAGE_SIZE,
                DeviceReadCallback, (void *)card, 0);
        if (libusb_submit_transfer(card->transfer) != 0)
        {
            SetError(card, "libusb_submit_transfer(): %s", ErrorString());
            return -1;
        }
        card->transferPending = TRUE;

        tv.tv_sec = 0;
        tv.tv_usec = 100;
        LockRelease(&(card->cardLock));
        rc = libusb_handle_events_timeout(libusbCxt, &tv);
        LockAcquire(&(card->cardLock));
        if (rc != 0)
        {
            SetError(card, "libusb_handle_events_timeout(): %s",
                    ErrorString());
            return -1;
        }

        return 1;
    }

    SetError(card, "libusb transfer failed: %s", ErrorString());
    return -1;
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
    int     bytesWritten;

    if (libusb_interrupt_transfer(card->cardHandle,
        LIBUSB_ENDPOINT_OUT | 1, (void *)buffer, 
        OPEN8055_HID_MESSAGE_SIZE, &bytesWritten, 0) == 0)
    {
        if (bytesWritten != OPEN8055_HID_MESSAGE_SIZE)
        {
            SetError(card, "short write - expected %d, wrote %d", 
                OPEN8055_HID_MESSAGE_SIZE, bytesWritten);
            return -1;
        }

        return bytesWritten;
    }

    SetError(card, "libusb_interrupt_transfer(): %s", ErrorString());
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
