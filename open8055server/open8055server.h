/* ----------------------------------------------------------------------
 * open8055server.h
 *
 *	Global definitions for the open8055server utility.
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

#ifndef _OPEN8055SERVER_H
#define _OPEN8055SERVER_H

/* ----------
 * Definitions
 * ----------
 */
#define			OPEN8055_SERVER_VERSION	"0.1.0"


#define			MAX_USERNAME		32
#define			MAX_CMDLINE			256
#define			MAX_CARDS			4

#define			LOG_FATAL			0x0001
#define			LOG_ERROR			0x0002
#define			LOG_WARN			0x0004
#define			LOG_INFO			0x0008
#define			LOG_DEBUG			0x0010

#define			LOG_CLIENT			0x0100
#define			LOG_CLIENTIO		0x0200
#define			LOG_CLIENTDEBUG		0x0400

#define			LOG_CARD			0x1000
#define			LOG_CARDIO			0x2000
#define			LOG_CARDDEBUG		0x4000

#define			LOG_DEFAULT			(LOG_ERROR | LOG_WARN | LOG_INFO)
#define			LOG_ALL				0xffff


#define			SERVER_MODE_RUN		0
#define			SERVER_MODE_STOP	1
#define			SERVER_MODE_RESTART	2

#define			CLIENT_MODE_RUN		0
#define			CLIENT_MODE_STOP	1
#define			CLIENT_MODE_STOPPED	2


typedef union
{
	struct sockaddr_in		ipv4;
	struct sockaddr_in6		ipv6;
} ClientAddr;

typedef struct ClientData
{
	int						sock;
	ClientAddr				addr;
	char					username[MAX_USERNAME + 1];

	char					read_buffer[MAX_CMDLINE];
	int						read_buffer_have;
	int						read_buffer_done;
	char					cmdline[MAX_CMDLINE];
	int						cmdline_have;

	int						mode;
	struct CardData		   *cards;

	pthread_t				thread;
	pthread_mutex_t			lock;
	struct ClientData	   *next;
} ClientData;

typedef struct CardData
{
	int						card_num;
	int						terminate;
	int						ioerror;
	ClientData			   *client;

	pthread_t				thread;
	pthread_mutex_t			lock;
	struct CardData		   *next;
} CardData;



/* ----------
 * Global data in open8055server.c
 * ----------
 */
extern int			server_log_mask;
extern pthread_t	server_thread;


/* ----------
 * Functions in open8055server.c
 * ----------
 */
extern void			server_log_func(char *fname, int lineno, 
						ClientData *client, int reason, char *fmt, ...);
#define				server_log(...)	\
						server_log_func(__FILE__, __LINE__, __VA_ARGS__)


/* ----------
 * Functions in client.c
 * ----------
 */
extern int			client_init(void);
extern int			client_shutdown(void);
extern void			client_catch_signal(int signum);

extern int			client_create(int sock, ClientAddr *addr);
extern int			client_reaper(void);


/* ----------
 * Functions in device_OS.c
 * ----------
 */
extern int			device_init(void);
extern void			device_exit(void);
extern int			device_present(int cardNumber);
extern int			device_open(int cardNumber);
extern int			device_close(int cardNumber);
extern int			device_read(int cardNumber, unsigned char *ioBuf);
extern int			device_write(int cardNumber, unsigned char *ioBuf);

extern char		   *device_error(int cardNumber);


#endif /* _OPEN8055SERVER_H */
