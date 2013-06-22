/* ----------------------------------------------------------------------
 * open8055server.c
 *
 *	A TCP/IP server for remote access to Open8055 USB Experiment Boards
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
int						server_log_mask = LOG_ALL;
pthread_t				server_thread;


/* ----------------------------------------------------------------------
 * Local data
 * ----------------------------------------------------------------------
 */
static int				server_mode = SERVER_MODE_RUN;
static int				server_sock4 = -1;
static int				server_sock6 = -1;
static int				server_port = 8055;
static fd_set			server_fdset;
static int				server_maxfd;


/* ----------------------------------------------------------------------
 * Local function declarations
 * ----------------------------------------------------------------------
 */
static int server_setup(void);
static int server_mainloop(void);
static int server_shutdown(void);
static void server_catch_signal(int signum);


/* ----------
 * main()
 * ----------
 */
int
main(const int argc, char * const argv[])
{
	int rc = 0;

	if (server_setup() != 0)
		return 2;
	if (server_mainloop() < 0)
		rc = 3;

	if (server_shutdown() < 0)
		if (rc == 0)
			rc = 4;

	if (server_mode == SERVER_MODE_RESTART)
	{
		server_log(NULL, LOG_INFO, "Open8055 network server restarting");
		execv(argv[0], argv);

		server_log(NULL, LOG_FATAL, "execv(): %s", strerror(errno));
		exit(5);
	}

	return rc;
}


/* ----------------------------------------------------------------------
 * Global functions
 * ----------------------------------------------------------------------
 */


/* ----------
 * server_log_func()
 * ----------
 */
void
server_log_func(char *fname, int lineno, 
		ClientData *client, int reason, char *fmt, ...)
{
	va_list		ap;
	char		buf[256];
	char		origin[256];
	char	   *reason_str;

	switch (reason)
	{
		case LOG_FATAL:			reason_str = "FATAL";		break;
		case LOG_ERROR:			reason_str = "ERROR";		break;
		case LOG_WARN:			reason_str = "WARN";		break;
		case LOG_INFO:			reason_str = "INFO";		break;
		case LOG_DEBUG:			reason_str = "DEBUG";		break;
		case LOG_CLIENT:		reason_str = "CLIENT";		break;
		case LOG_CLIENTIO:		reason_str = "CLIENTIO";	break;
		case LOG_CLIENTDEBUG:	reason_str = "CLIENTDEBUG";	break;
		case LOG_CARD:			reason_str = "CARD";		break;
		case LOG_CARDIO:		reason_str = "CARDIO";		break;
		case LOG_CARDDEBUG:		reason_str = "CARDDEBUG";	break;

		default:				reason_str = "UNKNOWN";
								reason = LOG_FATAL;
								break;
	}

	if ((reason & server_log_mask) == 0)
		return;

	if (client == NULL)
		strcpy(origin, "[main]");
	else
		snprintf(origin, sizeof(origin), "[%s@%s:%d]",
				client->username,
				(client->addr.ipv4.sin_family == AF_INET) ?
					inet_ntop(client->addr.ipv4.sin_family, 
							&(client->addr.ipv4.sin_addr), 
							buf, sizeof(buf))
					:
					inet_ntop(client->addr.ipv6.sin6_family, 
							&(client->addr.ipv6.sin6_addr), 
							buf, sizeof(buf)),
				ntohs(client->addr.ipv6.sin6_port));

	va_start(ap, fmt);

	fprintf(stdout, "%s %s (file %s line %d) ", reason_str, origin,
			fname, lineno);
	vfprintf(stdout, fmt, ap);
	fputc('\n', stdout);
	fflush(stdout);

	va_end(ap);
}


/* ----------------------------------------------------------------------
 * Local functions
 * ----------------------------------------------------------------------
 */


/* ----------
 * server_setup()
 * ----------
 */
static int
server_setup(void)
{
	struct sockaddr_in		addr4;
	struct sockaddr_in6		addr6;
	int						rc;
	int						on = 1;

	/* ----
	 * Try to create the IPV6 server socket.
	 * ----
	 */
	server_sock6 = socket(AF_INET6, SOCK_STREAM, 0);
	if (server_sock6 < 0)
	{
		server_log(NULL, LOG_WARN, "socket(AF_INET6): %s", strerror(errno));
	}
	else
	{
		rc = setsockopt(server_sock6, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		if (rc < 0)
		{
			server_log(NULL, LOG_FATAL, "setsockopt(SO_REUSEADDR): %s",
					strerror(errno));
			close(server_sock6);
			server_sock6 = -1;
			return -1;
		}

		memset(&addr6, 0, sizeof(addr4));
		addr6.sin6_family = AF_INET6;
		addr6.sin6_addr = in6addr_any;
		addr6.sin6_port = htons(server_port);
		rc = bind(server_sock6, (struct sockaddr *)&addr6, sizeof(addr6));
		if (rc < 0)
		{
			server_log(NULL, LOG_FATAL, "bind(AF_INET6): %s", strerror(errno));
			close(server_sock6);
			server_sock6 = -1;
			return -1;
		}

		rc = listen(server_sock6, 10);
		if (rc < 0)
		{
			server_log(NULL, LOG_FATAL, "listen(AF_INET6): %s", strerror(errno));
			close(server_sock6);
			server_sock6 = -1;
			return -1;
		}
	}

	/* ----
	 * Try to create the IPV4 server socket.
	 * ----
	 */
	server_sock4 = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock4 < 0)
	{
		server_log(NULL, LOG_WARN, "socket(AF_INET): %s", strerror(errno));
	}
	else
	{
		do {
			rc = setsockopt(server_sock4, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
			if (rc < 0)
			{
				server_log(NULL, LOG_WARN, "setsockopt(SO_REUSEADDR): %s",
						strerror(errno));
				close(server_sock4);
				server_sock4 = -1;
				break;
			}

			memset(&addr4, 0, sizeof(addr4));
			addr4.sin_family = AF_INET;
			addr4.sin_addr.s_addr = INADDR_ANY;
			addr4.sin_port = htons(server_port);
			rc = bind(server_sock4, (struct sockaddr *)&addr4, sizeof(addr4));
			if (rc < 0)
			{
				server_log(NULL, LOG_WARN, "bind(AF_INET): %s", strerror(errno));
				close(server_sock4);
				server_sock4 = -1;
				break;
			}

			rc = listen(server_sock4, 10);
			if (rc < 0)
			{
				server_log(NULL, LOG_WARN, "listen(AF_INET): %s", strerror(errno));
				close(server_sock4);
				server_sock4 = -1;
				break;
			}
		} while(0);
	}

	/* ----
	 * Check that we at least have one server socket.
	 * ----
	 */
	if (server_sock4 < 0 && server_sock6 < 0)
	{
		server_log(NULL, LOG_FATAL, "Could not create IPV4 or IPV6 server");
		return -1;
	}

	/* ----
	 * Set up the server select(2) fd_set and maxfd.
	 * ----
	 */
	FD_ZERO(&server_fdset);
	if (server_sock4 >= 0)
		FD_SET(server_sock4, &server_fdset);
	if (server_sock6 >= 0)
		FD_SET(server_sock6, &server_fdset);
	server_maxfd = (server_sock4 > server_sock6) ? server_sock4 : server_sock6;
	server_maxfd++;

	/* ----
	 * Record the main thread's ID.
	 * ----
	 */
	server_thread = pthread_self();

	/* ----
	 * Catch signals handled by the main thread.
	 * ----
	 */
	signal(SIGTERM, server_catch_signal);
	signal(SIGINT, server_catch_signal);
	signal(SIGHUP, server_catch_signal);

	/* ----
	 * Switch to RUN mode.
	 * ----
	 */
	server_mode = SERVER_MODE_RUN;

	server_log(NULL, LOG_INFO, "Open8055 network server ready");
	return 0;
}


/* ----------
 * server_mainloop()
 * ----------
 */
static int
server_mainloop(void)
{
	fd_set				rfds;
	struct timeval		tv;
	ClientAddr			addr;
	socklen_t			addrlen;
	int					sock;
	int					rc;

	while (server_mode == SERVER_MODE_RUN)
	{
		server_log(NULL, LOG_DEBUG, "Waiting for new client");

		/* ----
		 * Wait for something to do with a 10 second timeout.
		 * ----
		 */
		rfds = server_fdset;
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		rc = select(server_maxfd, &rfds, NULL, NULL, &tv);
		if (rc < 0)
		{
			if (errno == EINTR)
				continue;

			server_log(NULL, LOG_ERROR, "select(): %s", strerror(errno));
			server_mode = SERVER_MODE_RESTART;
			return -1;
		}

		/* ----
		 * Whatever just happened, clean up after closed connections first.
		 * ----
		 */
		client_reaper();

		/* ----
		 * If this was only a select(2) timeout, go wait for something new.
		 * ----
		 */
		if (rc == 0)
			continue;

		/* ----
		 * Handle new connection on IPV4 socket.
		 * ----
		 */
		if (server_sock4 >= 0 && FD_ISSET(server_sock4, &rfds))
		{
			memset(&addr, 0, (addrlen = sizeof(addr)));
			sock = accept(server_sock4, (struct sockaddr *)&addr, &addrlen);
			if (sock < 0)
			{
				server_log(NULL, LOG_ERROR, "accept(): %s", strerror(errno));
				server_mode = SERVER_MODE_RESTART;
				return -1;
			}

			if (client_create(sock, &addr) < 0)
			{
				write(sock, "ERROR Internal server error\n", 28);
				close(sock);
			}
		}

		/* ----
		 * Handle new connection on IPV6 socket.
		 * ----
		 */
		if (server_sock6 >= 0 && FD_ISSET(server_sock6, &rfds))
		{
			memset(&addr, 0, (addrlen = sizeof(addr)));
			sock = accept(server_sock6, (struct sockaddr *)&addr, &addrlen);
			if (sock < 0)
			{
				server_log(NULL, LOG_ERROR, "accept(): %s", strerror(errno));
				server_mode = SERVER_MODE_RESTART;
				return -1;
			}

			if (client_create(sock, &addr) < 0)
				close(sock);
		}
	}

	/* ----
	 * Server is done.
	 * ----
	 */
	server_log(NULL, LOG_INFO, "Exiting server_mainloop for %s",
			(server_mode == SERVER_MODE_STOP) ? "shutdown" : "restart");

	return 0;
}


/* ----------
 * server_shutdown()
 * ----------
 */
static int
server_shutdown(void)
{
	int		rc;

	if (server_sock4 >= 0)
	{
		close(server_sock4);
		server_sock4 = -1;
	}

	if (server_sock6 >= 0)
	{
		close(server_sock6);
		server_sock6 = -1;
	}

	rc = client_shutdown();

	server_log(NULL, LOG_INFO, "Open8055 network server shutdown complete");

	return rc;
}


/* ----------
 * server_catch_signal()
 * ----------
 */
static void
server_catch_signal(int signum)
{
	switch (signum)
	{
		case SIGHUP:	server_mode = SERVER_MODE_RESTART;
						break;

		default:		server_mode = SERVER_MODE_STOP;
						break;
	}
}


