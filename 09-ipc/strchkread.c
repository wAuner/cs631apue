/*	$NetBSD: strchkread.c,v 1.3 2003/08/07 10:30:50 agc Exp $
 *
 * Copyright (c) 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)strchkread.c	8.1 (Berkeley) 6/8/93
 */
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <netinet/in.h>

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BACKLOG 5

#ifndef SLEEP
#define SLEEP   5
#endif

void
handleConnection(int fd, struct sockaddr_in6 client)
{
	const char *rip;
	int rval;
	char claddr[INET6_ADDRSTRLEN];

	if ((rip = inet_ntop(PF_INET6, &(client.sin6_addr), claddr, INET6_ADDRSTRLEN)) == NULL) {
		perror("inet_ntop");
		rip = "unknown";
	} else {
		(void)printf("Client connection from %s!\n", rip);
	}

	do {
		char buf[BUFSIZ];
		bzero(buf, sizeof(buf));
		if ((rval = read(fd, buf, BUFSIZ)) < 0)
			perror("reading stream message");
		else if (rval == 0)
			printf("Ending connection from %s.\n", rip);
		else
			printf("Client (%s) sent: %s", rip, buf);
	} while (rval != 0);
	(void)close(fd);
}

/*
 * This program uses select() to check that someone is trying to connect
 * before calling accept().
 */
int main()
{
	int sock;
	socklen_t length;
	struct sockaddr_in6 server;

	memset(&server, 0, sizeof(server));

	if ((sock = socket(PF_INET6, SOCK_STREAM, 0)) < 0) {
		perror("opening stream socket");
		exit(EXIT_FAILURE);
		/* NOTREACHED */
	}

	server.sin6_family = PF_INET6;
	server.sin6_addr = in6addr_any;
	server.sin6_port = 0;
	if (bind(sock, (struct sockaddr *)&server, sizeof(server)) != 0) {
		perror("binding stream socket");
		exit(EXIT_FAILURE);
		/* NOTREACHED */
	}

	/* Find out assigned port number and print it out */
	length = sizeof(server);
	if (getsockname(sock, (struct sockaddr *)&server, &length) != 0) {
		perror("getting socket name");
		exit(EXIT_FAILURE);
		/* NOTREACHED */
	}
	(void)printf("Socket has port #%d\n", ntohs(server.sin6_port));

	if (listen(sock, BACKLOG) < 0) {
		perror("listening");
		exit(EXIT_FAILURE);
		/* NOTREACHED */
	}

	for (;;) {
		fd_set ready;
		struct timeval to;

		FD_ZERO(&ready);
		FD_SET(sock, &ready);
		to.tv_sec = SLEEP;
		to.tv_usec = 0;
		if (select(sock + 1, &ready, 0, 0, &to) < 0) {
			perror("select");
			continue;
		}
		if (FD_ISSET(sock, &ready)) {
			int fd;
			struct sockaddr_in6 client;
			memset(&client, 0, sizeof(client));

			length = sizeof(client);
			if ((fd = accept(sock, (struct sockaddr *)&client, &length)) < 0) {
				perror("accept");
				continue;
			}

			handleConnection(fd, client);
		} else {
			(void)printf("Idly sitting here, waiting for connections...\n");
		}
	}

	/* NOTREACHED */
}
