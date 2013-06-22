/* ----------------------------------------------------------------------
 * client.c
 *
 *	Client handling part of the Open8055server.
 *
 * ----------------------------------------------------------------------
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
 * ----------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "open8055.h"
#include "open8055server.h"


/* ----------------------------------------------------------------------
 * Global data
 * ----------------------------------------------------------------------
 */


/* ----------------------------------------------------------------------
 * Local data
 * ----------------------------------------------------------------------
 */
static ClientData	   *client_list = NULL;


/* ----------------------------------------------------------------------
 * Local function declarations
 * ----------------------------------------------------------------------
 */


/* ----------------------------------------------------------------------
 * Global functions
 * ----------------------------------------------------------------------
 */


/* ----------
 * client_create()
 * ----------
 */
int
client_create(int sock, ClientAddr *addr)
{
	write(sock, "client_create(): not implemented\n", 33);
	return -1;
}


/* ----------
 * client_reaper()
 * ----------
 */
int
client_reaper(void)
{
	return 0;
}


/* ----------
 * client_shutdown()
 * ----------
 */
int
client_shutdown(void)
{
	return 0;
}


/* ----------------------------------------------------------------------
 * Local functions
 * ----------------------------------------------------------------------
 */


