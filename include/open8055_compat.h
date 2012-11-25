#ifndef _OPEN8055_COMPAT_H
#define _OPEN8055_COMPAT_H

#ifdef _WIN32

#include <winsock2.h>
#include <windows.h>
#include <basetyps.h>
#include <setupapi.h>
#include <rpcdce.h>
#include <io.h>

typedef int		socklen_t;

#else /* !_WIN32 */

#include <unistd.h>
#include <errno.h>
#include <libusb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

typedef int			SOCKET;
typedef struct sockaddr		SOCKADDR;
typedef struct sockaddr_in	SOCKADDR_IN;
#define INVALID_SOCKET		-1

#define closesocket(_s)		close(_s)
#define stricmp(_s1,_s2)	strcasecmp((_s1),(_s2))
#define WSACleanup()		{}

#endif /* _WIN32 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <signal.h>
#include <sys/time.h>
#include <getopt.h>
#include <math.h>
#include <unistd.h>

#endif /* _OPEN8055_COMPAT_H */
