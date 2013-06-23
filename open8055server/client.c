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
static pthread_mutex_t	client_list_lock;


/* ----------------------------------------------------------------------
 * Local function declarations
 * ----------------------------------------------------------------------
 */
static void *client_thread_run(void *cdata);
static void client_command(ClientData *client);
static int client_send(ClientData *client, char *fmt, ...);


/* ----------------------------------------------------------------------
 * Global functions
 * ----------------------------------------------------------------------
 */


/* ----------
 * client_init()
 * ----------
 */
int
client_init(void)
{
	if (pthread_mutex_init(&client_list_lock, NULL) != 0)
	{
		server_log(NULL, LOG_FATAL, "pthread_mutex_init(): %s",
				strerror(errno));
		return -1;
	}

	return 0;
}


/* ----------
 * client_shutdown()
 *
 * 	Terminate all existing client connections for server shutdown by
 * 	sending them SIGUSR2 and waiting for them to exit.
 * ----------
 */
int
client_shutdown(void)
{
	ClientData *next;

	pthread_mutex_lock(&client_list_lock);

	while (client_list != NULL)
	{
		server_log(client_list, LOG_INFO, "terminating client connection");

		if (pthread_kill(client_list->thread, SIGUSR2) != 0)
			server_log(client_list, LOG_ERROR, "pthread_kill(): %s",
					strerror(errno));
		if (pthread_join(client_list->thread, NULL) != 0)
			server_log(client_list, LOG_ERROR, "pthread_join(): %s",
					strerror(errno));
		if (pthread_mutex_destroy(&(client_list->lock)) != 0)
			server_log(client_list, LOG_ERROR, "pthread_mutex_destroy(): %s",
					strerror(errno));
		
		next = client_list->next;
		free(client_list);
		client_list = next;
	}

	return 0;
}


/* ----------
 * client_catch_signal()
 * ----------
 */
void
client_catch_signal(int signum)
{
	/* ----
	 * Nothing to do here. The only signal we ever receive
	 * is SIGUSR2 and that is handled in client_command().
	 * ----
	 */
}


/* ----------
 * client_create()
 * ----------
 */
int
client_create(int sock, ClientAddr *addr)
{
	ClientData	   *client;

	/* ----
	 * Allocate the client data structure
	 * ----
	 */
	client = (ClientData *)malloc(sizeof(ClientData));
	if (client == NULL)
	{
		server_log(NULL, LOG_FATAL, "malloc(): %s", strerror(errno));
		close(sock);
		return -1;
	}
	memset(client, 0, sizeof(ClientData));

	/* ----
	 * Initialize data and mutex
	 * ----
	 */
	client->sock = sock;
	memcpy(&(client->addr), addr, sizeof(ClientAddr));
	client->username[0] = '-';

	if (pthread_mutex_init(&(client->lock), NULL) != 0)
	{
		server_log(client, LOG_FATAL, "pthread_mutex_init(): %s", 
				strerror(errno));
		close(sock);
		free(client);
		return -1;
	}

	/* ----
	 * Send the HELLO and SALT messages from here.
	 * TODO: implement the salt part.
	 * ----
	 */
	if (client_send(client, "HELLO Open8055server %s\nSALT %s\n", 
			OPEN8055_SERVER_VERSION, 
			"00000000000000000000000000000000") < 0)
	{
		if (client->sock >= 0)
			close(sock);
			pthread_mutex_destroy(&(client->lock));
			free(client);
			return -1;
	}

	/* ----
	 * Start the client thread
	 * ----
	 */
	if (pthread_create(&(client->thread), NULL, client_thread_run, client) != 0)
	{
		server_log(client, LOG_FATAL, "pthread_create(): %s", 
				strerror(errno));
		pthread_mutex_destroy(&(client->lock));
		close(sock);
		free(client);
		return -1;
	}

	/*
	 * Add the new client to the list of clients
	 * ----
	 */
	pthread_mutex_lock(&client_list_lock);
	client->next = client_list;
	client_list = client;
	pthread_mutex_unlock(&client_list_lock);

	return 0;
}


/* ----------
 * client_reaper()
 * ----------
 */
int
client_reaper(void)
{
	ClientData	  **cpp;
	ClientData	   *client;

	server_log(NULL, LOG_CLIENTDEBUG, "client_reaper() called");

	/* ----
	 * Search for STOPPED clients.
	 * ----
	 */
	pthread_mutex_lock(&client_list_lock);

	cpp = &client_list;

	while (*cpp != NULL)
	{
		client = *cpp;

		pthread_mutex_lock(&(client->lock));
		if (client->mode == CLIENT_MODE_STOPPED)
		{
			/* ----
			 * Client is stopped, reap it
			 * ----
			 */
			server_log(client, LOG_CLIENTDEBUG, "client is stopped");
			
			/* ----
			 * Remove client from the list
			 * ----
			 */
			*cpp = client->next;

			/* ----
			 * Join the thread
			 * ----
			 */
			if (pthread_join(client->thread, NULL) != 0)
			{
				server_log(client, LOG_FATAL, "pthread_join(): %s",
						strerror(errno));
				pthread_kill(server_thread, SIGHUP);
			}
			
			pthread_mutex_destroy(&(client->lock));
			free(client);
		}
		else
		{
			/* ----
			 * Client still running. Advance to next client in the list.
			 * ----
			 */
			pthread_mutex_unlock(&(client->lock));
			cpp = &(client->next);
		}
	}

	pthread_mutex_unlock(&client_list_lock);

	return 0;
}


/* ----------------------------------------------------------------------
 * Local functions
 * ----------------------------------------------------------------------
 */


/* ----------
 * client_thread()
 * ----------
 */
static void *
client_thread_run(void *cdata)
{
	ClientData	   *client = (ClientData *)cdata;

	server_log(client, LOG_ERROR, "client_thread_run()");

	/* ----
	 * Client main loop.
	 * ----
	 */
	for (;;)
	{
		pthread_mutex_lock(&(client->lock));
		if (client->mode == CLIENT_MODE_STOP)
		{
			pthread_mutex_unlock(&(client->lock));
			break;
		}
		pthread_mutex_unlock(&(client->lock));

		client_command(client);
	}

	/* ----
	 * Exit the client thread. Close connection, close open cards
	 * and set mode to STOPPED.
	 * ----
	 */
	pthread_mutex_lock(&(client->lock));

	if (client->sock >= 0)
		close(client->sock);
	client->sock = -1;
	client->mode = CLIENT_MODE_STOPPED;

	pthread_mutex_unlock(&(client->lock));

	/* ----
	 * Signal the main thread to call the reaper.
	 * ----
	 */
	server_log(client, LOG_CLIENTDEBUG, "stopped");
	pthread_kill(server_thread, SIGUSR1);

	return NULL;
}


/* ----------
 * client_command()
 *
 * 	Process one client command. Wait for data if necessary.
 * ----------
 */
static void
client_command(ClientData *client)
{
	fd_set			rfds;
	sigset_t		sigmask;
	int				rc;

	while (client->read_buffer_have > client->read_buffer_done)
	{
		char *cp = &(client->cmdline[client->cmdline_have]);

		*cp = client->read_buffer[client->read_buffer_done++];
		if (*cp == '\r')
			continue;

		/* ----
		 * Check for complete command line
		 * ----
		 */
		if (*cp == '\n')
		{
			/* ----
			 * Process this one command.
			 * ----
			 */
			*cp = '\0';
			server_log(client, LOG_CLIENTIO, "RECV '%s'",
					client->cmdline);

			/* ----
			 * Reset command line and return to client main loop.
			 * ----
			 */
			client->cmdline_have = 0;
			return;
		}

		/* ----
		 * No LF seen yet, check for too long command line.
		 * ----
		 */
		client->cmdline_have++;
		if (client->cmdline_have >= MAX_CMDLINE)
		{
			/* ----
			 * This is a misbehaving client. Terminate connection.
			 * ----
			 */
			server_log(client, LOG_ERROR, "client sent too long command line");
			pthread_mutex_lock(&(client->lock));
			client->mode = CLIENT_MODE_STOP;
			pthread_mutex_unlock(&(client->lock));
			return;
		}
	}

	if (client->sock < 0)
		return;

	/* ----
	 * We ran out of data. Read more from the remote.
	 * Allow SIGUSR2 to be delivered during this time only.
	 * ----
	 */
	FD_ZERO(&rfds);
	FD_SET(client->sock, &rfds);

	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGUSR2);
	pthread_sigmask(SIG_UNBLOCK, &sigmask, NULL);

	rc = select(client->sock + 1, &rfds, NULL, NULL, NULL);
	
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGUSR2);
	pthread_sigmask(SIG_BLOCK, &sigmask, NULL);

	/* ----
	 * Handle SIGUSR2 and errors
	 * ----
	 */
	if (rc < 0)
	{
		if (errno == EINTR)
		{
			server_log(client, LOG_CLIENTDEBUG, "SIGUSR2 received");
			client_send(client, "E Server shutdown or restart\n");
		}
		else
			server_log(client, LOG_ERROR, "select(): %s", strerror(errno));

		pthread_mutex_lock(&(client->lock));
		client->mode = CLIENT_MODE_STOP;
		pthread_mutex_unlock(&(client->lock));
		return;
	}

	if (!FD_ISSET(client->sock, &rfds))
	{
		server_log(client, LOG_WARN, "select() returned %d "
				"but client socket not ready", rc);
		return;
	}

	rc = recv(client->sock, client->read_buffer, MAX_CMDLINE, 0);
	if (rc < 0)
	{
		server_log(client, LOG_ERROR, "recv(): %s", strerror(errno));

		pthread_mutex_lock(&(client->lock));
		client->mode = CLIENT_MODE_STOP;
		close(client->sock);
		client->sock = -1;
		pthread_mutex_unlock(&(client->lock));
		return;
	}

	if (rc == 0)
	{
		server_log(client, LOG_CLIENTDEBUG, "recv(): EOF");

		pthread_mutex_lock(&(client->lock));
		client->mode = CLIENT_MODE_STOP;
		close(client->sock);
		client->sock = -1;
		pthread_mutex_unlock(&(client->lock));
		return;
	}

	client->read_buffer_have = rc;
	client->read_buffer_done = 0;
}


/* ----
 * client_send()
 * ----
 */
static int
client_send(ClientData *client, char *fmt, ...)
{
	va_list		ap;
	char		buf[256];
	ssize_t		len;
	ssize_t		rc;

	if (client->sock < 0)
		return -1;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	len = strlen(buf);

	pthread_mutex_lock(&(client->lock));
	rc = send(client->sock, buf, len, 0);
	pthread_mutex_unlock(&(client->lock));

	if (rc != len)
	{
		server_log(client, LOG_ERROR, "send(): %s", strerror(errno));

		pthread_mutex_lock(&(client->lock));
		client->mode = CLIENT_MODE_STOP;
		close(client->sock);
		client->sock = -1;
		pthread_mutex_unlock(&(client->lock));
		return -1;
	}

	server_log(client, LOG_CLIENTIO, "SEND '%s'", buf);

	return rc;
}


