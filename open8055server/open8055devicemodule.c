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


/* ----
 * Local definitions
 * ----
 */
#define OPEN8055_GUID   "4d1e55b2-f16f-11cf-88cb-001111000030"
#define OPEN8055_VID    0x10cf
#define OPEN8055_PID    0x55f0


/* ----
 * Local functions
 * ----
 */
static int device_init(void);
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
static libusb_context		   *libusbCxt;
static libusb_device_handle	   *cardHandle[OPEN8055_MAX_CARDS];
static int						hadKernelDriver[OPEN8055_MAX_CARDS];
static int						initialized = 0;


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
	int		rc;
	char	errbuf[1024];

	if (initialized)
	{
		PyErr_SetString(PyExc_RuntimeError, 
				"device_init() called multiple times");
		return -1;
	}

	if ((rc = libusb_init(&libusbCxt)) != 0)
	{
		snprintf(errbuf, sizeof(errbuf),
				"libusb_init() failed - %s", libusb_strerror(rc));
		PyErr_SetString(PyExc_RuntimeError, errbuf);
		return -1;
	}

	initialized = 1;
	return 0;
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

	if (!initialized)
	{
		PyErr_SetString(PyExc_RuntimeError, 
				"device_present() called before initialized");
		return NULL;
	}

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
	if (cardHandle[cardNumber] != NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "card already open");
		return NULL;
	}

	if (device_present(self, args) == NULL)
		return NULL;

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
		return NULL;
	}
	else
	{
		hadKernelDriver[cardNumber] = rc;
		if (rc != 0)
		{
			if ((rc = libusb_detach_kernel_driver(dev, interface)) != 0)
			{
				snprintf(errbuf, sizeof(errbuf),
						"libusb_detach_kernel_driver() failed - %s", 
						libusb_strerror(rc));
				PyErr_SetString(PyExc_RuntimeError, errbuf);
				libusb_close(dev);
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
		if (hadKernelDriver[cardNumber])
			libusb_attach_kernel_driver(dev, interface);
		libusb_close(dev);
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
		if (hadKernelDriver[cardNumber])
			libusb_attach_kernel_driver(dev, interface);
		libusb_close(dev);
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
		if (hadKernelDriver[cardNumber])
			libusb_attach_kernel_driver(dev, interface);
		libusb_close(dev);
		return NULL;
	}

	cardHandle[cardNumber] = dev;

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
	if (cardHandle[cardNumber] == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "card not open");
		return NULL;
	}

	libusb_release_interface(cardHandle[cardNumber], interface);
	if (hadKernelDriver[cardNumber])
		libusb_attach_kernel_driver(cardHandle[cardNumber], interface);
	libusb_close(cardHandle[cardNumber]);

	cardHandle[cardNumber] = NULL;

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
	int				rc;
	int				bytesRead;
	unsigned char	ioBuf[OPEN8055_HID_MESSAGE_SIZE];
	char			data[OPEN8055_HID_MESSAGE_SIZE * 2 + 1];
	char		   *cp;
	int				idx;
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
	if (cardHandle[cardNumber] == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "card not open");
		return NULL;
	}

	if ((rc = libusb_interrupt_transfer(cardHandle[cardNumber],
			LIBUSB_ENDPOINT_IN | 1, ioBuf, 
			OPEN8055_HID_MESSAGE_SIZE, &bytesRead, 0)) == 0)
	{
		if (bytesRead != OPEN8055_HID_MESSAGE_SIZE)
		{
			snprintf(errbuf, sizeof(errbuf),
					"short read - expected %d, got %d", 
					OPEN8055_HID_MESSAGE_SIZE, bytesRead);
			PyErr_SetString(PyExc_IOError, errbuf);
			return NULL;
		}

		cp = data;
		for (idx = 0; idx < OPEN8055_HID_MESSAGE_SIZE; idx++)
		{
			sprintf(cp, "%02X", ioBuf[idx] & 0xFF);
			cp += 2;
		}

		return Py_BuildValue("s", data);
	}

	snprintf(errbuf, sizeof(errbuf),
			"libusb_interrupt_transfer(): %s", 
			libusb_strerror(rc));
	PyErr_SetString(PyExc_IOError, errbuf);
	return NULL;
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
	int				bytesWritten;
	const char	   *data;
	unsigned char	ioBuf[OPEN8055_HID_MESSAGE_SIZE];
	int				idx;
	char			errbuf[1024];
	int				rc;

	/* ----
	 * Parse command args and check card number
	 * ----
	 */
	if (!PyArg_ParseTuple(args, "is", &cardNumber, &data))
		return NULL;
	if (cardNumber < 0 || cardNumber >= OPEN8055_MAX_CARDS)
	{
		PyErr_SetString(PyExc_ValueError, "invalid card number");
		return NULL;
	}
	if (cardHandle[cardNumber] == NULL)
	{
		PyErr_SetString(PyExc_RuntimeError, "card not open");
		return NULL;
	}

	/* ----
	 * Parse the HID packet data into raw bytes
	 * ----
	 */
	if (data == NULL || strlen(data) > OPEN8055_HID_MESSAGE_SIZE * 2)
	{
		PyErr_SetString(PyExc_ValueError, "invalid HID report");
		return NULL;
	}
	idx = 0;
	memset(ioBuf, 0, sizeof(ioBuf));
	while (data[0] != '\0')
	{
		unsigned int	bval;

		if (data[1] == '\0')
		{
			PyErr_SetString(PyExc_ValueError, "invalid HID report");
			return NULL;
		}

		if (sscanf(data, "%02x", &bval) != 1)
		{
			PyErr_SetString(PyExc_ValueError, "invalid HID report");
			return NULL;
		}

		ioBuf[idx++] = bval;
		data += 2;
	}

	/* ----
	 * Send it via interrupt transfer.
	 * ----
	 */
	if ((rc = libusb_interrupt_transfer(cardHandle[cardNumber],
			LIBUSB_ENDPOINT_OUT | 1, ioBuf, 
			OPEN8055_HID_MESSAGE_SIZE, &bytesWritten, 0)) == 0)
	{
		if (bytesWritten != OPEN8055_HID_MESSAGE_SIZE)
		{
			snprintf(errbuf, sizeof(errbuf),
					"short write - expected %d, got %d", 
					OPEN8055_HID_MESSAGE_SIZE, bytesWritten);
			PyErr_SetString(PyExc_IOError, errbuf);
			return NULL;
		}

		bytesWritten++;

		return Py_BuildValue("i", bytesWritten);
	}

	snprintf(errbuf, sizeof(errbuf),
			"libusb_interrupt_transfer() failed - %s", 
			libusb_strerror(rc));
	PyErr_SetString(PyExc_IOError, errbuf);
	return NULL;
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


#if 0
/* ----------------------------------------------------------------
 * device_unix.c
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include "open8055.h"
#include "open8055_compat.h"
#include "open8055_common.h"
#include "open8055_hid_protocol.h"
#include "open8055server.h"


/* ----
 * Local functions
 * ----
 */
static void set_error(int cardNumber, char *fmt, ...);

#ifndef HAVE_LIBUSB_STRERROR
static char *libusb_strerror(int errnum);
#endif


/* ----
 * Local data
 * ----
 */
static libusb_context		   *libusbCxt;
static libusb_device_handle	   *cardHandle[MAX_CARDS];
static int						hadKernelDriver[MAX_CARDS];
static int						initialized = 0;
static char						lastError[MAX_CARDS][1024];
static char						globalError[1024];
static pthread_mutex_t			errorLock;



/* ----
 * device_init()
 *
 *	Initialize libusb.
 * ----
 */
int
device_init(void)
{
	int rc;

	if (initialized)
		return 0;

	if ((rc = libusb_init(&libusbCxt)) != 0)
	{
		set_error(-1, "libusb_init(): %s", libusb_strerror(rc));
		return -1;
	}

	if (pthread_mutex_init(&errorLock, NULL) != 0)
	{
		set_error(-1, "pthread_mutex_init(): %s", strerror(errno));
		libusb_exit(libusbCxt);
		return -1;
	}

	initialized = 1;

	return 0;
}


/* ----
 * device_exit()
 *
 *	Finish libusb access.
 * ----
 */
void
device_exit(void)
{
	if (initialized)
		libusb_exit(libusbCxt);

	initialized = 0;
}


/* ----
 * device_present()
 *
 *	Check if a given Open8055 card is present in the system without
 *	actually opening it.
 * ----
 */
int
device_present(int cardNumber)
{
	int								cardFound = 0;
	int								numDevices;
	struct libusb_device		  **deviceList;
	struct libusb_device_descriptor	deviceDesc;
	int								i;

	/* ----
	 * Get the list of USB devices in the system.
	 * ----
	 */
	numDevices = libusb_get_device_list(libusbCxt, &deviceList);
	if (numDevices < 0)
	{
		set_error(cardNumber, "libusb_get_device_list(): %s", strerror(errno));
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
			set_error(cardNumber, "libusb_get_device_descriptor(): %s", 
					strerror(errno));
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
 * device_open()
 *
 *	Open a specific Open8055 by board number.
 * ----
 */
int
device_open(int cardNumber)
{
	libusb_device_handle   *dev;
	int						rc;
	int						interface = 0;

	if (cardNumber < 0 || cardNumber >= MAX_CARDS)
	{
		set_error(-1, "card %d not possible", cardNumber);
		return -1;
	}

	if (cardHandle[cardNumber] != NULL)
	{
		set_error(cardNumber, "card %d already open", cardNumber);
		return -1;
	}

	if (!device_present(cardNumber))
	{
		set_error(cardNumber, "card %d not present", cardNumber);
		return -1;
	}

	/* ----
	 * Open the device.
	 * ----
	 */
	dev = libusb_open_device_with_vid_pid(
			libusbCxt, OPEN8055_VID, OPEN8055_PID + cardNumber);
	if (dev == NULL)
	{
		set_error(cardNumber, "libusb_open_device_with_vid_pid(): failed");
		return -1;
	}

	/* ----
	 * If a kernel driver is active, detach it.
	 * ----
	 */
	if ((rc = libusb_kernel_driver_active(dev, interface)) < 0)
	{
		set_error(cardNumber, "libusb_kernel_driver_active(): %s", 
				libusb_strerror(rc));
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
				set_error(cardNumber, "libusb_detach_kernel_driver(): %s", 
						strerror(errno));
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
		set_error(cardNumber, "libusb_set_configuration(): %s", 
				strerror(errno));
		libusb_close(dev);
		return -1;
	}

	/* ----
	 * Claim the interface.
	 * ----
	 */
	if (libusb_claim_interface(dev, interface) != 0)
	{
		set_error(cardNumber, "libusb_claim_interface(): %s", strerror(errno));
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
		set_error(cardNumber, "libusb_set_interface_alt_setting(): %s", 
				strerror(errno));
		libusb_release_interface(dev, interface);
		if (hadKernelDriver[cardNumber])
			libusb_attach_kernel_driver(dev, interface);
		libusb_close(dev);
		return -1;
	}

	cardHandle[cardNumber] = dev;

	return 0;
}


/* ----
 * device_close()
 *
 *	Close the specified card.
 * ----
 */
int
device_close(int cardNumber)
{
	int		interface = 0;

	if (cardNumber < 0 || cardNumber >= MAX_CARDS)
	{
		set_error(-1, "card %d not possible", cardNumber);
		return -1;
	}

	if (cardHandle[cardNumber] == NULL)
	{
		set_error(cardNumber, "card %d not open", cardNumber);
		return -1;
	}

	libusb_release_interface(cardHandle[cardNumber], interface);
	if (hadKernelDriver[cardNumber])
		libusb_attach_kernel_driver(cardHandle[cardNumber], interface);
	libusb_close(cardHandle[cardNumber]);

	cardHandle[cardNumber] = NULL;

	return 0;
}


/* ----
 * device_read()
 *
 *	Receive one message from the Open8055.
 * ----
 */
int
device_read(int cardNumber, unsigned char *ioBuf)
{
	int		rc;
	int		bytesRead;

	if (cardNumber < 0 || cardNumber >= MAX_CARDS)
	{
		set_error(-1, "card %d not possible", cardNumber);
		return -1;
	}

	if (cardHandle[cardNumber] == NULL)
	{
		set_error(cardNumber, "card %d not open", cardNumber);
		return -1;
	}

	if ((rc = libusb_interrupt_transfer(cardHandle[cardNumber],
			LIBUSB_ENDPOINT_IN | 1, ioBuf, 
			OPEN8055_HID_MESSAGE_SIZE, &bytesRead, 0)) == 0)
	{
		if (bytesRead != OPEN8055_HID_MESSAGE_SIZE)
		{
			set_error(cardNumber, "short read - expected %d, got %d", 
					OPEN8055_HID_MESSAGE_SIZE, bytesRead);
			return -1;
		}

		return bytesRead;
	}

	set_error(cardNumber, "libusb_interrupt_transfer(): %s", 
			libusb_strerror(rc));
	return -1;
}


/* ----
 * device_write()
 *
 *	Send one message to the Open8055.
 * ----
 */
int
device_write(int cardNumber, unsigned char *ioBuf)
{
	int		bytesWritten;

	if (cardNumber < 0 || cardNumber >= MAX_CARDS)
	{
		set_error(-1, "card %d not possible", cardNumber);
		return -1;
	}

	if (cardHandle[cardNumber] == NULL)
	{
		set_error(cardNumber, "card %d not open", cardNumber);
		return -1;
	}

	if (libusb_interrupt_transfer(cardHandle[cardNumber],
			LIBUSB_ENDPOINT_OUT | 1, ioBuf, 
			OPEN8055_HID_MESSAGE_SIZE, &bytesWritten, 0) == 0)
	{
		if (bytesWritten != OPEN8055_HID_MESSAGE_SIZE)
		{
			set_error(cardNumber, "short write - expected %d, got %d", 
					OPEN8055_HID_MESSAGE_SIZE, bytesWritten);
			return -1;
		}

		bytesWritten++;

		return bytesWritten;
	}

	set_error(cardNumber, "libusb_interrupt_transfer(): %s", strerror(errno));
	return -1;
}


/* ----
 * device_error()
 * ----
 */
char *
device_error(int cardNumber)
{
	if (cardNumber < 0 || cardNumber >= MAX_CARDS)
		return globalError;
	else
		return lastError[cardNumber];
}


/* ----
 * set_error()
 *
 *	Set the error message for a card.
 * ----
 */
static void
set_error(int cardNumber, char *fmt, ...)
{
	va_list		ap;
	char	   *dest;

	if (cardNumber < 0 || cardNumber >= MAX_CARDS)
		dest = globalError;
	else
		dest = lastError[cardNumber];

	pthread_mutex_lock(&errorLock);

	va_start(ap, fmt);
	vsnprintf(dest, 1024, fmt, ap);
	va_end(ap);

	pthread_mutex_unlock(&errorLock);
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

#endif
