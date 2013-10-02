/* ----------------------------------------------------------------------
 * open8055_compat.h
 *
 *  Definitions to make differences between Unix and Windows easier.
 * ----------------------------------------------------------------------
 *
 *  Copyright (c) 2012, Jan Wieck
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

#ifndef _OPEN8055_COMPAT_H
#define _OPEN8055_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

#include <winsock2.h>
#include <windows.h>
#include <basetyps.h>
#include <setupapi.h>
#include <rpcdce.h>
#include <io.h>

typedef int     socklen_t;

#ifndef htons
#define htons(_val) ((((_val) & 0xff00) >> 8) | (((_val) & 0x00ff) << 8))
#endif
#ifndef ntohs
#define ntohs(_val) ((((_val) & 0xff00) >> 8) | (((_val) & 0x00ff) << 8))
#endif


#else /* !_WIN32 */

#include <unistd.h>
#include <errno.h>
#include <libusb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

typedef int                 SOCKET;
typedef struct sockaddr     SOCKADDR;
typedef struct sockaddr_in  SOCKADDR_IN;
#define INVALID_SOCKET      -1

#define closesocket(_s)     close(_s)
#define stricmp(_s1,_s2)    strcasecmp((_s1),(_s2))
#define WSACleanup()        {}

#endif /* _WIN32 */


#include <stdio.h>
#include <stdlib.h>
//#include <stdint.h>
#include <stdarg.h>
#include <string.h>
//#include <strings.h>
#include <ctype.h>
#include <signal.h>
//#include <sys/time.h>
//#include <getopt.h>
#include <math.h>
//#include <unistd.h>

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE ~FALSE
#endif

#ifdef __cplusplus
}
#endif

#endif /* _OPEN8055_COMPAT_H */
