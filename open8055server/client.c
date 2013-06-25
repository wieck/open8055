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

#include "config.h"

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

#include "open8055_compat.h"
#include "open8055_common.h"
#include "open8055_hid_protocol.h"
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
static int client_command_parse(ClientData *client);
static int client_send(ClientData *client, int dolock, char *fmt, ...);
static void client_card_open(ClientData *client, int card_num);
static void client_card_close(ClientData *client, int card_num);
static void *client_card_thread(void *cdata);

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

	/* ----
	 * Terminate all open client connections.
	 * ----
	 */
	while (client_list != NULL)
	{
		server_log(client_list, LOG_INFO, "terminating client connection "
				"due to shutdown/restart");

		/* ----
		 * Set the flag in the client data signaling the client thread
		 * to terminate.
		 * ----
		 */
		pthread_mutex_lock(&(client_list->lock));
		client_list->mode = CLIENT_MODE_STOP;
		pthread_mutex_unlock(&(client_list->lock));

		/* ----
		 * Send SIGUSR2 to the client thread to inform it to
		 * check flags.
		 * ----
		 */
		if (pthread_kill(client_list->thread, SIGUSR2) != 0)
			server_log(client_list, LOG_ERROR, "pthread_kill(): %s",
					strerror(errno));

		/* ----
		 * Wait for the client thead to finish and free resources.
		 * ----
		 */
		if (pthread_join(client_list->thread, NULL) != 0)
			server_log(client_list, LOG_ERROR, "pthread_join(): %s",
					strerror(errno));
		if (pthread_mutex_destroy(&(client_list->lock)) != 0)
			server_log(client_list, LOG_ERROR, "pthread_mutex_destroy(): %s",
					strerror(errno));

		server_log(client_list, LOG_CLIENTDEBUG, "client is reaped");
		
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
	if (client_send(client, TRUE,  "HELLO Open8055server %s\nSALT %s\n", 
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
			server_log(client, LOG_CLIENTDEBUG, "client is reaped");
			
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

	server_log(client, LOG_CLIENT, "connected");

	/* ----
	 * Client main loop.
	 * ----
	 */
	for (;;)
	{
		/* ----
		 * Check for terminate flag.
		 * ----
		 */
		pthread_mutex_lock(&(client->lock));
		if (client->mode == CLIENT_MODE_STOP)
		{
			pthread_mutex_unlock(&(client->lock));
			break;
		}
		pthread_mutex_unlock(&(client->lock));

		client_command(client);
	}

	pthread_mutex_lock(&(client->lock));

	/* ----
	 * Client exit time. Close all open cards.
	 * ----
	 */
	while (client->cards != NULL)
	{
		int		card_num = client->cards->card_num;

		pthread_mutex_unlock(&(client->lock));
		client_card_close(client, card_num);
		pthread_mutex_lock(&(client->lock));
	}

	/* ----
	 * Close the connection and set mode to STOPPED.
	 * ----
	 */

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
			server_log(client, LOG_CLIENTIO, "RECV %s",
					client->cmdline);
			if (client_command_parse(client) < 0)
			{
				pthread_mutex_lock(&(client->lock));
				client->mode = CLIENT_MODE_STOP;
				pthread_mutex_unlock(&(client->lock));
			}

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
			CardData   *card;

			server_log(client, LOG_CLIENTDEBUG, "SIGUSR2 received");
			pthread_mutex_lock(&(client->lock));

			if (client->mode == CLIENT_MODE_STOP)
			{
				client_send(client, FALSE, "ERROR Server shutdown or restart\n");
				if (client->sock >= 0)
				{
					close(client->sock);
					client->sock = -1;
				}
				pthread_mutex_unlock(&(client->lock));
				client->mode = CLIENT_MODE_STOP;
				pthread_mutex_unlock(&(client->lock));
				return;
			}
			
			card = client->cards;
			while (card != NULL)
			{
				CardData   *next = card->next;

				if (card->ioerror)
				{
					server_log(client, LOG_CLIENTDEBUG, "card %d: close "
							"due to IO error", card->card_num);
					client_card_close(client, card->card_num);
				}

				card = next;
			}

			return;
		}

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
 * client_command_parse()
 * ----
 */
static int
client_command_parse(ClientData *client)
{
	char	response[MAX_CMDLINE];
	char	cmdcopy[MAX_CMDLINE];
	char   *sepptr = cmdcopy;
	char   *cmd_token;

	strcpy(cmdcopy, client->cmdline);

	cmd_token = strsep(&sepptr, " \t");

	/* ----
	 * LOGIN command
	 * ----
	 */
	if (stricmp(cmd_token, "LOGIN") == 0)
	{
		char	*username;
		char	*password;

		if (strcmp(client->username, "-") != 0)
		{
			client_send(client, TRUE, "LOGIN ERROR Already logged in\n");
			return 0;
		}

		username = strsep(&sepptr, " \t");
		password = strsep(&sepptr, " \t");

		if (username == NULL)
		{
			client_send(client, TRUE, "ERROR Usage: "
					"LOGIN username [password]\n");
			return 0;
		}
		if (strcmp(username, "-") == 0 || strlen(username) > MAX_USERNAME)
		{
			client_send(client, TRUE, "LOGIN ERROR Illegal username\n");
			return 0;
		}

		/*
		 * TODO: handle actual login by checking username and password
		 */
		
		strcpy(client->username, username);
		server_log(client, LOG_CLIENT, "Authenticated user='%s'", username);
		client_send(client, TRUE, "LOGIN OK\n");
		return 0;
	}
	/* ----
	 * BYE command
	 * ----
	 */
	else if (stricmp(cmd_token, "BYE") == 0)
	{
		pthread_mutex_lock(&(client->lock));
		client_send(client, FALSE, "BYE\n");
		if (client->sock >= 0)
		{
			close(client->sock);
			client->sock = -1;
		}
		pthread_mutex_unlock(&(client->lock));

		return -1;
	}

	/* ----
	 * For everything else, the client must be logged in.
	 * ----
	 */
	if (strcmp(client->username, "-") == 0)
	{
		client_send(client, TRUE, "ERROR Not logged in\n");
		return 0;
	}

	/* ----
	 * LIST command
	 *
	 * 	Returns a list of connected cards
	 * ----
	 */
	if (stricmp(cmd_token, "LIST") == 0)
	{
		int		card_num;
		int		rc;

		strcpy(response, "LIST OK");
		for (card_num = 0; card_num < MAX_CARDS; card_num++)
		{
			rc = device_present(card_num);
			if (rc < 0)
			{
				server_log(client, LOG_ERROR, "device_present(): %s",
						device_error(card_num));
				return 0;
			}

			if (rc)
				sprintf(response + strlen(response), " %d", card_num);
		}
		client_send(client, TRUE, "%s\n", response);
		return 0;
	}

	/* ----
	 * OPEN command
	 * ----
	 */
	else if (stricmp(cmd_token, "OPEN") == 0)
	{
		int		card_num;
		char   *cardstr;

		if ((cardstr = strsep(&sepptr, " \t")) == NULL || *cardstr == '\0')
		{
			client_send(client, TRUE, "ERROR Usage: OPEN card\n");
			return 0;
		}
		if (sscanf(cardstr, "%d", &card_num) != 1)
		{
			client_send(client, TRUE, "OPEN %s ERROR Invalid card number\n", 
					cardstr);
			return 0;
		}
		if (card_num < 0 || card_num >= MAX_CARDS)
		{
			client_send(client, TRUE, "OPEN %s ERROR Invalid card number\n", 
					cardstr);
			return 0;
		}

		client_card_open(client, card_num);
	}

	/* ----
	 * CLOSE command
	 * ----
	 */
	else if (stricmp(cmd_token, "CLOSE") == 0)
	{
		int		card_num;
		char   *cardstr;

		if ((cardstr = strsep(&sepptr, " \t")) == NULL || *cardstr == '\0')
		{
			client_send(client, TRUE, "ERROR Usage: CLOSE card\n");
			return 0;
		}
		if (sscanf(cardstr, "%d", &card_num) != 1)
		{
			client_send(client, TRUE, "CLOSE %s ERROR Invalid card number\n", 
					cardstr);
			return 0;
		}
		if (card_num < 0 || card_num >= MAX_CARDS)
		{
			client_send(client, TRUE, "CLOSE %s ERROR Invalid card number\n", 
					cardstr);
			return 0;
		}

		client_card_close(client, card_num);
	}

	/* ----
	 * SEND command
	 * ----
	 */
	else if (stricmp(cmd_token, "SEND") == 0)
	{
		CardData	   *card;
		char		   *cardstr;
		char		   *hexdata;
		int				card_num;
		unsigned char	packet[OPEN8055_HID_MESSAGE_SIZE];
		unsigned char  *pcp;

		cardstr = strsep(&sepptr, " \t");
		hexdata = strsep(&sepptr, " \t");

		/* ----
		 * Get the card address and hex packet tokens
		 * ----
		 */
		if (cardstr == NULL || hexdata == NULL)
		{
			client_send(client, TRUE, "ERROR Usage: SEND card hexdata\n");
			return 0;
		}

		/* ----
		 * Parse the card address
		 * ----
		 */
		if (sscanf(cardstr, "%d", &card_num) != 1)
		{
			client_send(client, TRUE, "SEND %s ERROR Invalid card number\n", 
					cardstr);
			return 0;
		}
		if (card_num < 0 || card_num >= MAX_CARDS)
		{
			client_send(client, TRUE, "SEND %s ERROR Invalid card number\n", 
					cardstr);
			return 0;
		}

		/* ----
		 * Parse the hex data
		 * ----
		 */
		if (strlen(hexdata) > OPEN8055_HID_MESSAGE_SIZE * 2)
		{
			client_send(client, TRUE, "SEND %s ERROR Invalid packet data\n", 
					cardstr);
			return 0;
		}
		memset(packet, 0, sizeof(packet));
		pcp = packet;
		while (*hexdata != '\0')
		{
			int		byte;

			if (sscanf(hexdata, "%02x", &byte) != 1)
			{
				client_send(client, TRUE, "SEND %s ERROR Invalid packet data\n", 
						cardstr);
				return 0;
			}
			*pcp++ = byte & 0xFF;
			hexdata += 2;
		}

		/* ----
		 * Find the open card to send to.
		 * ----
		 */
		for (card = client->cards; card != NULL; card = card->next)
		{
			if (card->card_num == card_num)
				break;
		}
		if (card == NULL)
		{
			client_send(client, TRUE, "SEND %d ERROR Card not open\n",
					card_num);
			return 0;
		}

		if (device_write(card_num, packet) < 0)
		{
			server_log(client, LOG_ERROR, "card %d: device_write(): %s",
					card_num, device_error(card_num));
			client_send(client, TRUE, "SEND %d ERROR %s\n",
					card_num, device_error(card_num));
			client_card_close(client, card_num);
		}

		return 0;
	}

	/* ----
	 * Invalid command tag
	 * ----
	 */
	else 
		client_send(client, TRUE, "ERROR Command not implemented yet\n");
	return 0;
}


/* ----
 * client_send()
 * ----
 */
static int
client_send(ClientData *client, int dolock, char *fmt, ...)
{
	va_list		ap;
	char		buf[1024];
	ssize_t		len;
	ssize_t		rc;

	if (client->sock < 0)
		return -1;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	len = strlen(buf);

	if (dolock)
		pthread_mutex_lock(&(client->lock));
	rc = send(client->sock, buf, len, 0);
	if (dolock)
		pthread_mutex_unlock(&(client->lock));

	if (rc != len)
	{
		server_log(client, LOG_ERROR, "send(): %s", strerror(errno));

		if (dolock)
			pthread_mutex_lock(&(client->lock));
		client->mode = CLIENT_MODE_STOP;
		close(client->sock);
		client->sock = -1;
		if (dolock)
			pthread_mutex_unlock(&(client->lock));
		return -1;
	}

	if (len > 0 && buf[len - 1] == '\n')
		len--;
	server_log(client, LOG_CLIENTIO, "SEND '%.*s'", len, buf);

	return rc;
}


/* ----------
 * client_card_open()
 * ----------
 */
static void
client_card_open(ClientData *client, int card_num)
{
	CardData   *card;

	/* ----
	 * Try to open the physical card
	 * ----
	 */
	if (device_open(card_num) != 0)
	{
		client_send(client, TRUE, "OPEN %d ERROR %s\n", card_num, 
				device_error(card_num));
		return;
	}

	/* ----
	 * Allocate and initialize the card data structure
	 * ----
	 */
	card = (CardData *)malloc(sizeof(CardData));
	if (card == NULL)
	{
		server_log(client, LOG_FATAL, "malloc(): %s", strerror(errno));
		pthread_kill(server_thread, SIGHUP);
		device_close(card_num);
		return;
	}
	memset(card, 0, sizeof(CardData));

	card->card_num	= card_num;
	card->client	= client;
	
	/* ----
	 * Create the card structure access lock
	 * ----
	 */
	if (pthread_mutex_init(&(card->lock), NULL) != 0)
	{
		server_log(client, LOG_ERROR, "pthread_mutex_init(): %s",
				strerror(errno));
		client_send(client, TRUE, "OPEN %d ERROR Internal server error\n", 
				card_num);
		free(card);
		return;
	}

	/* ----
	 * Create the thread that reads HID messages from the card.
	 * ----
	 */
	if (pthread_create(&(card->thread), NULL, client_card_thread, card) != 0)
	{
		pthread_mutex_destroy(&(card->lock));
		server_log(client, LOG_ERROR, "pthread_create(): %s",
				strerror(errno));
		client_send(client, TRUE, "OPEN %d ERROR Internal server error\n", 
				card_num);
		free(card);
		return;
	}

	/* ----
	 * Success. Add the card to the list of open cards for this client
	 * and report OK.
	 * ----
	 */
	card->next = client->cards;
	client->cards = card;

	server_log(client, LOG_CARDIO, "card %d opened", card_num);
	client_send(client, TRUE, "OPEN %d OK\n", card_num);
}


/* ----------
 * client_card_close()
 * ----------
 */
static void
client_card_close(ClientData *client, int card_num)
{
	Open8055_hidMessage_t	msg;
	CardData			   *card;
	CardData			  **cpp;

	pthread_mutex_lock(&(client->lock));
	cpp = &(client->cards);
	while (*cpp != NULL)
	{
		card = client->cards;

		if (card->card_num == card_num)
			break;
		
		cpp = &(card->next);
	}
	if (*cpp == NULL)
	{
		pthread_mutex_unlock(&(client->lock));
		client_send(client, TRUE, "CLOSE %d ERROR Card not open\n", 
				card_num);
		return;
	}

	/* ----
	 * Remove the card from the list of open cards for this client
	 * and unlock the client.
	 * ----
	 */
	*cpp = card->next;
	pthread_mutex_unlock(&(client->lock));

	/* ----
	 * Flag the card reader thread that it should terminate.
	 * ----
	 */
	pthread_mutex_lock(&(card->lock));
	card->terminate = TRUE;

	/* ----
	 * If the card is still known to be OK, send a message to the 
	 * card that forces an input report.
	 * ----
	 */
	if (!card->ioerror)
	{
		memset(&msg, 0, sizeof(msg));
		msg.msgType = OPEN8055_HID_MESSAGE_GETINPUT;
		if (device_write(card->card_num, (unsigned char *)&msg) < 0)
		{
			server_log(client, LOG_FATAL, "card %d: %s", card_num,
					device_error(card_num));
			client_send(client, TRUE, "CLOSE %d ERROR %s\n", card_num,
					device_error(card_num));
			card->ioerror = TRUE;
		}
	}

	pthread_mutex_unlock(&(card->lock));
	
	/* ----
	 * Wait for the reader thread to terminate. Even if the above
	 * hid message failed, the thread should terminate now on its
	 * own because it too should get an IO error.
	 * ----
	 */
	if (pthread_join(card->thread, NULL) != 0)
	{
		server_log(client, LOG_ERROR, "card %d: pthread_join(): %s",
				card_num, strerror(errno));
		client_send(client, TRUE, "CLOSE %d ERROR pthread_join(): %s\n",
				card_num, strerror(errno));

		return;
	}
	
	/* ----
	 * Inform the client about card closed
	 * ----
	 */
	if (!card->ioerror)
		client_send(client, TRUE, "CLOSE %d OK\n", card_num);

	/* ----
	 * Close the physical device.
	 * ----
	 */
	if (device_close(card_num) != 0)
	{
		server_log(client, LOG_ERROR, "card %d device_close(): %s",
				card_num, device_error(card_num));
		client_send(client, TRUE, "CLOSE %d ERROR %s\n", 
				card_num, device_error(card_num));
		pthread_mutex_destroy(&(card->lock));
		free(card);

		return;
	}

	/* ----
	 * Final cleanup.
	 * ----
	 */
	pthread_mutex_destroy(&(card->lock));
	server_log(client, LOG_CARDIO, "card %d closed", card_num);
	free(card);
}


/* ----------
 * client_card_tread()
 * ----------
 */
static void *
client_card_thread(void *cdata)
{
	CardData	   *card = (CardData *)cdata;
	ClientData	   *client = card->client;
	unsigned char	hidbuf[OPEN8055_HID_MESSAGE_SIZE];
	char			hexbuf[OPEN8055_HID_MESSAGE_SIZE * 2 + 1];
	int				i;
	char		   *cp;

	for (;;)
	{
		pthread_mutex_lock(&(card->lock));
		if (card->terminate)
		{
			pthread_mutex_unlock(&(card->lock));
			break;
		}
		pthread_mutex_unlock(&(card->lock));

		if (device_read(card->card_num, hidbuf) < 0)
		{
			server_log(client, LOG_ERROR, "card %d: %s", card->card_num,
					device_error(card->card_num));
			client_send(client, TRUE, "RECV %d ERROR %s\n", card->card_num,
					device_error(card->card_num));
			pthread_mutex_lock(&(card->lock));
			card->ioerror = 1;
			pthread_kill(client->thread, SIGUSR2);
			pthread_mutex_unlock(&(card->lock));
			break;
		}

		cp = hexbuf;
		for (i = 0; i < OPEN8055_HID_MESSAGE_SIZE; i++)
		{
			sprintf(cp, "%02X", hidbuf[i]);
			cp += 2;
		}
		*cp = '\0';

		server_log(client, LOG_CARDIO, "card %d RECV %s", card->card_num,
				hexbuf);
		client_send(client, TRUE, "RECV %d DATA %s\n", card->card_num, 
				hexbuf);
	}

	server_log(client, LOG_CARDIO, "card %d IO thread exiting", card->card_num);

	return NULL;
}


