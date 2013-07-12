/* ----------------------------------------------------------------
 * open8055devicemodule.c
 *
 *		Unix implementation of USB IO for the Open8055server.
 *
 * ----------------------------------------------------------------
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
 * ----------------------------------------------------------------
 */

#include <Python.h>

#include "config.h"

#include "open8055_compat.h"
#include "open8055_common.h"
#include "open8055_hid_protocol.h"

#include <stdio.h>
#include <string.h>
#include <pthread.h>


/* ----
 * Local definitions
 * ----
 */
#define OPEN8055_GUID   "4d1e55b2-f16f-11cf-88cb-001111000030"
#define OPEN8055_VID    0x10cf
#define OPEN8055_PID    0x55f0

typedef struct Open8055Card
{
	pthread_mutex_t			lock;
	libusb_device_handle   *handle;
	int						hadKernelDriver;
	struct libusb_transfer *readTransfer;
	int						readCompleted;
	struct libusb_transfer *writeTransfer;
	int						writeCompleted;
} Open8055Card;


/* ----
 * Local functions
 * ----
 */
static int device_init(void);
static int device_handle_events(Open8055Card *card, int *complete);
static void device_transfer_callback(struct libusb_transfer *transfer);
static PyObject *device_present(PyObject *self, PyObject *args);
static PyObject *device_open(PyObject *self, PyObject *args);
static PyObject *device_close(PyObject *self, PyObject *args);
static PyObject *device_read(PyObject *self, PyObject *args);
static PyObject *device_write(PyObject *self, PyObject *args);

#ifndef HAVE_LIBUSB_STRERROR
static char *libusb_strerror(int errnum);
#endif


/* ----
 * Local data
 * ----
 */
static int				initialized = 0;
static libusb_context  *libusbCxt;
pthread_mutex_t			deviceGlobalLock = PTHREAD_MUTEX_INITIALIZER;
Open8055Card			deviceCard[OPEN8055_MAX_CARDS];


static PyMethodDef device_methods[] = {
	{"present", device_present, METH_VARARGS, 
			"Check if a specific Open8055 is present"},
	{"open", device_open, METH_VARARGS, 
			"Open a Open8055"},
	{"close", device_close, METH_VARARGS, 
			"Close a Open8055"},
	{"read", device_read, METH_VARARGS, 
			"Read an HID report from a Open8055"},
	{"write", device_write, METH_VARARGS, 
			"Write an HID report to a Open8055"},
	{NULL, NULL, 0, NULL}
};


/* ----
 * device_init()
 *
 *	Initialize libusb.
 * ----
 */
static int
device_init(void)
{
	int		i;
	int		rc;
	char	errbuf[1024];

	pthread_mutex_lock(&deviceGlobalLock);

	if (initialized)
	{
		pthread_mutex_unlock(&deviceGlobalLock);
		return 0;
	}

	if ((rc = libusb_init(&libusbCxt)) != 0)
	{
		snprintf(errbuf, sizeof(errbuf),
				"libusb_init() failed - %s", libusb_strerror(rc));
		PyErr_SetString(PyExc_RuntimeError, errbuf);
		pthread_mutex_unlock(&deviceGlobalLock);
		return -1;
	}

	for (i = 0; i < OPEN8055_MAX_CARDS; i++)
	{
		if (pthread_mutex_init(&(deviceCard[i].lock), NULL) != 0)
		{
			PyErr_SetString(PyExc_RuntimeError, "pthread_mutex_init() failed");
			return -1;
		}
	}

	initialized = 1;
	pthread_mutex_unlock(&deviceGlobalLock);
	return 0;
}


/* ----
 * device_handle_events()
 *
 *	Function called by device_read() and device_write() to perform
 *	the multi-thread correct event handling.
 * ----
 */
static int
device_handle_events(Open8055Card *card, int *completed)
{
	int				rc = 0;
	struct timeval	tv;

	Py_BEGIN_ALLOW_THREADS

	while (!*completed && rc == 0)
	{
		if (libusb_try_lock_events(libusbCxt) == 0)
		{
			/* ----
			 * We got the event lock, so we are the event handler.
			 * ----
			 */
			while(!*completed)
			{
				if (!libusb_event_handling_ok(libusbCxt))
				{
					libusb_unlock_events(libusbCxt);
					break;
				}

				tv.tv_sec = 0;
				tv.tv_usec = 0;
				rc = libusb_handle_events_locked(libusbCxt, &tv);
				if (rc != 0)
					break;
			}
			libusb_unlock_events(libusbCxt);
		}
		else
		{
			/* ----
			 * Somebody else is handling events ... wait for them.
			 * ----
			 */
			libusb_lock_event_waiters(libusbCxt);
			while (!completed)
			{
				/* ----
				 * Now that we have the waiters lock, recheck that
				 * somebody is really processing events.
				 * ----
				 */
				if (!libusb_event_handler_active(libusbCxt))
					break;
				
				/* ----
				 * OK to really wait now.
				 * ----
				 */
				libusb_wait_for_event(libusbCxt, NULL);
			}
			libusb_unlock_event_waiters(libusbCxt);
		}
	}

	Py_END_ALLOW_THREADS

	return rc;
}


/* ----
 * device_transfer_callback()
 *
 *	Function called by libusb when a transfer has finished.
 * ----
 */
static void
device_transfer_callback(struct libusb_transfer *transfer)
{
	int *completed = (int *)(transfer->user_data);

	*completed = TRUE;
}


/* ----
 * device_present()
 *
 *	Check if a given Open8055 card is present in the system without
 *	actually opening it.
 * ----
 */
static PyObject *
device_present(PyObject *self, PyObject *args)
{
	int								cardNumber = 0;
	int								cardFound = 0;
	int								numDevices;
	struct libusb_device		  **deviceList;
	struct libusb_device_descriptor	deviceDesc;
	int								i;
	char							errbuf[1024];

	/* ----
	 * Parse command args
	 * ----
	 */
	if (!PyArg_ParseTuple(args, "i", &cardNumber))
		return NULL;

	/* ----
	 * Get the list of USB devices in the system.
	 * ----
	 */
	numDevices = libusb_get_device_list(libusbCxt, &deviceList);
	if (numDevices < 0)
	{
		snprintf(errbuf, sizeof(errbuf),
				"libusb_get_device_list() failed - %s", 
				libusb_strerror(numDevices));
		PyErr_SetString(PyExc_RuntimeError, errbuf);
		return NULL;
	}

	/* ----
	 * See if any of them is the Open8055 we are looking for.
	 * ----
	 */
	for (i = 0; i < numDevices; i++)
	{
		if (libusb_get_device_descriptor(deviceList[i], &deviceDesc) != 0)
		{
			snprintf(errbuf, sizeof(errbuf),
					"libusb_get_device_descriptor() failed - %s", 
					libusb_strerror(numDevices));
			PyErr_SetString(PyExc_RuntimeError, errbuf);
			libusb_free_device_list(deviceList, 1);
			return NULL;
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
	return Py_BuildValue("i", cardFound);
}


/* ----
 * device_open()
 *
 *	Open a specific Open8055 by board number.
 * ----
 */
static PyObject *
device_open(PyObject *self, PyObject *args)
{
	int						cardNumber;
	Open8055Card		   *card;
	libusb_device_handle   *dev;
	int						rc;
	int						interface = 0;
	char					errbuf[1024];

	/* ----
	 * Parse command arguments
	 * ----
	 */
	if (!PyArg_ParseTuple(args, "i", &cardNumber))
		return NULL;
	if (cardNumber < 0 || cardNumber >= OPEN8055_MAX_CARDS)
	{
		PyErr_SetString(PyExc_ValueError, "invalid card number");
		return NULL;
	}

	/* ----
	 * Sanity checks
	 * ----
	 */
	if (deviceCard[cardNumber].handle != NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "card already open");
		return NULL;
	}

	if (device_present(self, args) == NULL)
		return NULL;

	/* ----
	 * Allocate the libusb asynchronous transfer structures.
	 * ----
	 */
	card = &deviceCard[cardNumber];
	if ((card->readTransfer = libusb_alloc_transfer(0)) == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "libusb_alloc_transfer() failed");
		return NULL;
	}
	if ((card->writeTransfer = libusb_alloc_transfer(0)) == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "libusb_alloc_transfer() failed");
		libusb_free_transfer(card->readTransfer);
		return NULL;
	}

	/* ----
	 * Open the device.
	 * ----
	 */
	dev = libusb_open_device_with_vid_pid(
			libusbCxt, OPEN8055_VID, OPEN8055_PID + cardNumber);
	if (dev == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError,
				"libusb_open_device_with_vid_pid() failed");
		libusb_free_transfer(card->readTransfer);
		libusb_free_transfer(card->writeTransfer);
		return NULL;
	}

	/* ----
	 * If a kernel driver is active, detach it.
	 * ----
	 */
	if ((rc = libusb_kernel_driver_active(dev, interface)) < 0)
	{
		snprintf(errbuf, sizeof(errbuf),
				"libusb_kernel_driver_active() failed - %s", 
				libusb_strerror(rc));
		PyErr_SetString(PyExc_RuntimeError, errbuf);
		libusb_close(dev);
		libusb_free_transfer(card->readTransfer);
		libusb_free_transfer(card->writeTransfer);
		return NULL;
	}
	else
	{
		deviceCard[cardNumber].hadKernelDriver = rc;
		if (rc != 0)
		{
			if ((rc = libusb_detach_kernel_driver(dev, interface)) != 0)
			{
				snprintf(errbuf, sizeof(errbuf),
						"libusb_detach_kernel_driver() failed - %s", 
						libusb_strerror(rc));
				PyErr_SetString(PyExc_RuntimeError, errbuf);
				libusb_close(dev);
				libusb_free_transfer(card->readTransfer);
				libusb_free_transfer(card->writeTransfer);
				return NULL;
			}
		}
	}

	/* ----
	 * Set configuration
	 * ----
	 */
	if (libusb_set_configuration(dev, 1) != 0)
	{
		snprintf(errbuf, sizeof(errbuf),
				"libusb_set_configuration() failed - %s", 
				libusb_strerror(rc));
		PyErr_SetString(PyExc_RuntimeError, errbuf);
		if (deviceCard[cardNumber].hadKernelDriver)
			libusb_attach_kernel_driver(dev, interface);
		libusb_close(dev);
		libusb_free_transfer(card->readTransfer);
		libusb_free_transfer(card->writeTransfer);
		return NULL;
	}

	/* ----
	 * Claim the interface.
	 * ----
	 */
	if (libusb_claim_interface(dev, interface) != 0)
	{
		snprintf(errbuf, sizeof(errbuf),
				"libusb_claim_interface() failed - %s", 
				libusb_strerror(rc));
		PyErr_SetString(PyExc_RuntimeError, errbuf);
		if (deviceCard[cardNumber].hadKernelDriver)
			libusb_attach_kernel_driver(dev, interface);
		libusb_close(dev);
		libusb_free_transfer(card->readTransfer);
		libusb_free_transfer(card->writeTransfer);
		return NULL;
	}

	/* ----
	 * Activate alternate setting
	 * ----
	 */
	if (libusb_set_interface_alt_setting(dev, interface, 0) != 0)
	{
		snprintf(errbuf, sizeof(errbuf),
				"libusb_set_interface_alt_setting() failed - %s", 
				libusb_strerror(rc));
		PyErr_SetString(PyExc_RuntimeError, errbuf);
		libusb_release_interface(dev, interface);
		if (deviceCard[cardNumber].hadKernelDriver)
			libusb_attach_kernel_driver(dev, interface);
		libusb_close(dev);
		libusb_free_transfer(card->readTransfer);
		libusb_free_transfer(card->writeTransfer);
		return NULL;
	}

	deviceCard[cardNumber].handle = dev;

	return Py_BuildValue("i", cardNumber);
}


/* ----
 * device_close()
 *
 *	Close the specified card.
 * ----
 */
static PyObject *
device_close(PyObject *self, PyObject *args)
{
	int		cardNumber = 0;
	int		interface = 0;

	/* ----
	 * Parse command arguments
	 * ----
	 */
	if (!PyArg_ParseTuple(args, "i", &cardNumber))
		return NULL;
	if (cardNumber < 0 || cardNumber >= OPEN8055_MAX_CARDS)
	{
		PyErr_SetString(PyExc_ValueError, "invalid card number");
		return NULL;
	}
	if (deviceCard[cardNumber].handle == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "card not open");
		return NULL;
	}

	libusb_free_transfer(deviceCard[cardNumber].readTransfer);
	libusb_free_transfer(deviceCard[cardNumber].writeTransfer);
	libusb_release_interface(deviceCard[cardNumber].handle, interface);
	if (deviceCard[cardNumber].hadKernelDriver)
		libusb_attach_kernel_driver(deviceCard[cardNumber].handle, interface);
	libusb_close(deviceCard[cardNumber].handle);

	deviceCard[cardNumber].handle = NULL;

	return Py_BuildValue("i", cardNumber);
}


/* ----
 * device_read()
 *
 *	Receive one message from the Open8055.
 * ----
 */
static PyObject *
device_read(PyObject *self, PyObject *args)
{
	int				cardNumber;
	Open8055Card   *card;
	int				rc;
	unsigned char	ioBuf[OPEN8055_HID_MESSAGE_SIZE];
	char			errbuf[1024];


	/* ----
	 * Parse command arguments
	 * ----
	 */
	if (!PyArg_ParseTuple(args, "i", &cardNumber))
		return NULL;
	if (cardNumber < 0 || cardNumber >= OPEN8055_MAX_CARDS)
	{
		PyErr_SetString(PyExc_ValueError, "invalid card number");
		return NULL;
	}
	if (deviceCard[cardNumber].handle == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "card not open");
		return NULL;
	}
	card = &deviceCard[cardNumber];

	libusb_fill_interrupt_transfer(card->readTransfer, card->handle,
			LIBUSB_ENDPOINT_IN | 1, ioBuf, 
			OPEN8055_HID_MESSAGE_SIZE,
			device_transfer_callback, (void *)&(card->readCompleted), 0);
	card->readCompleted = FALSE;
	if ((rc = libusb_submit_transfer(card->readTransfer)) != 0)
	{
		snprintf(errbuf, sizeof(errbuf),
				"libusb_submit_transfer(): %s", 
				libusb_strerror(rc));
		PyErr_SetString(PyExc_IOError, errbuf);
		return NULL;
	}

	if ((rc = device_handle_events(card, &(card->readCompleted))) < 0)
	{
		snprintf(errbuf, sizeof(errbuf),
				"device_handle_transfer(): %s", 
				libusb_strerror(rc));
		PyErr_SetString(PyExc_IOError, errbuf);
		return NULL;
	}

	if (card->readTransfer->status != LIBUSB_TRANSFER_COMPLETED)
	{
		PyErr_SetString(PyExc_IOError, 
				"asynchronous transfer failed: unknown reason");
		return NULL;
	}

	return Py_BuildValue("s#", &ioBuf, OPEN8055_HID_MESSAGE_SIZE);
}


/* ----
 * device_write()
 *
 *	Send one message to the Open8055.
 * ----
 */
static PyObject *
device_write(PyObject *self, PyObject *args)
{
	int				cardNumber;
	Open8055Card   *card;
	unsigned char  *data;
	unsigned char	ioBuf[OPEN8055_HID_MESSAGE_SIZE];
	int				data_len;
	char			errbuf[1024];
	int				rc;

	/* ----
	 * Parse command args and check card number
	 * ----
	 */
	if (!PyArg_ParseTuple(args, "is#", &cardNumber, 
			&data, &data_len))
		return NULL;

	if (cardNumber < 0 || cardNumber >= OPEN8055_MAX_CARDS)
	{
		PyErr_SetString(PyExc_ValueError, "invalid card number");
		return NULL;
	}
	if (deviceCard[cardNumber].handle == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "card not open");
		return NULL;
	}
	card = &deviceCard[cardNumber];

	/* ----
	 * Send it via interrupt transfer.
	 * ----
	 */
	memset(ioBuf, 0, sizeof(ioBuf));
	memcpy(ioBuf, data, (data_len > sizeof(ioBuf)) ? sizeof(ioBuf) : data_len);
	libusb_fill_interrupt_transfer(card->writeTransfer, card->handle,
			LIBUSB_ENDPOINT_OUT | 1, ioBuf, 
			OPEN8055_HID_MESSAGE_SIZE,
			device_transfer_callback, (void *)&(card->writeCompleted), 0);
	card->writeCompleted = FALSE;
	if ((rc = libusb_submit_transfer(card->writeTransfer)) != 0)
	{
		snprintf(errbuf, sizeof(errbuf),
				"libusb_submit_transfer(): %s", 
				libusb_strerror(rc));
		PyErr_SetString(PyExc_IOError, errbuf);
		return NULL;
	}

	if (device_handle_events(card, &(card->writeCompleted)) < 0)
	{
		snprintf(errbuf, sizeof(errbuf),
				"device_handle_transfer(): %s", 
				libusb_strerror(rc));
		PyErr_SetString(PyExc_IOError, errbuf);
		return NULL;
	}

	if (card->writeTransfer->status != LIBUSB_TRANSFER_COMPLETED)
	{
		snprintf(errbuf, sizeof(errbuf),
				"asynchronous transfer failed: %s", 
				libusb_strerror(rc));
		PyErr_SetString(PyExc_IOError, errbuf);
		return NULL;
	}

	return Py_BuildValue("i", 0);
}


#ifndef HAVE_LIBUSB_STRERROR
static char libusb_strerror_message[64];
static char *
libusb_strerror(int errnum)
{
	sprintf(libusb_strerror_message, "libusb-error = %d", errnum);
	return libusb_strerror_message;
}
#endif


PyMODINIT_FUNC
initopen8055device(void)
{
	if (Py_InitModule("open8055device", device_methods) == NULL)
		return;

	device_init();
}


