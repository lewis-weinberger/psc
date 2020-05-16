#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "msg.h"
#include "error.h"

struct sockaddr_un addr;
int nclient, sfd, *cfd;
char path[SUN_PATH_MAX];
fd_set readfds;

/*
 * Start the server-side ipc.
 * nc -- number of clients expected to join.
 * sp -- filepath of the (Unix domain) socket to create.
 * Note: must call sstop() when finished, to clean up.
 */
void sstart(int nc, char *sp)
{
	int i;
	char welcome[1024];
	
	nclient = nc;
	strncpy(path, sp, SUN_PATH_MAX - 1);
	printf("Server starting, expecting %d client(s)\n", nclient);

	/* Allocate client sockets */
	cfd = emalloc(nclient * sizeof(int));
	memset(cfd, 0, nclient * sizeof(int));

	/* Create socket */
	if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("socket error");
		exit(-1);
	}

	/* Bind socket to path */
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, sizeof(addr.sun_path));
	unlink(path);
	if (bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		perror("bind error");
		exit(-1);
	}
	printf("* Listening on socket: %s\n", path);

	/* Specify maximum number of connections */
	if (listen(sfd, nclient) < 0)
	{
		perror("listen error");
		exit(-1);
	}

	/* Wait for all connections */
	printf("* Waiting for client connections...\n");
	for (i = 0; i < nclient; i++)
	{
		/* Accept any incoming connection on master socket */
		if ((cfd[i] = accept(sfd, NULL, NULL)) < 0)
		{
			perror("accept error");
			exit(-1);
		}
		printf("* Client %d connected!\n", i);
		
		/* Send welcome message */
		strncpy(welcome, "[Server] Welcome!", sizeof(welcome));
		stoc(i, welcome, sizeof(welcome));
	}
}

/*
 * Start the client-side ipc.
 * sp -- filepath of the (Unix domain) socket to connect to.
 * Note: must call cstop() when finished, to clean up.
 */
void cstart(char *sp)
{
	char welcome[1024];
	
	strncpy(path, sp, SUN_PATH_MAX - 1);
	printf("Client starting\n");

	/* Create socket */
	if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("socket error");
		exit(-1);
	}
	
	/* Connect to server */
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, sizeof(addr.sun_path));
	if (connect(sfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		perror("connect error");
		exit(-1);
	}
	printf("* Connected to: %s\n", path);
	
	/* Read welcome message from server */
	cfroms(welcome, sizeof(welcome));
	printf("%s\n", welcome);
}

/*
 * Send message from server to client.
 * client -- ID of client to send to.
 * msg -- buffer holding message contents.
 * len -- number of bytes of message to send.
  * Returns: number of bytes sent.
 * Note: msg buffer must be at least len bytes long.
 */
ssize_t stoc(int client, void *msg, size_t len)
{
	ssize_t n;
	
	if ((n = write(cfd[client], msg, len)) < 0)
	{
		perror("read error");
		exit(-1);
	}
	return n;
}

/*
 * Send message from client to server.
 * msg -- buffer holding message contents.
 * len -- number of bytes of message to send.
 * Returns: number of bytes sent.
 * Note: msg buffer must be at least len bytes long.
 */
ssize_t ctos(void *msg, size_t len)
{
	ssize_t n;
	
	if ((n = write(sfd, msg, len)) < 0)
	{
		perror("read error");
		exit(-1);
	}
	return n;
}

/*
 * Receive message (as server) from client.
 * client -- ID of client to receive from.
 * msg -- pre-allocated buffer to hold message.
 * len -- number of bytes of message to read.
 * Returns: number of bytes received.
 * Note: msg buffer must have at least len bytes allocated.
 */
ssize_t sfromc(int client, void *msg, size_t len)
{
	ssize_t n;
	
	if ((n = read(cfd[client], msg, len)) < 0)
	{
		perror("read error");
		exit(-1);
	}
	return n;
}

/*
 * Receive message (as client) from server.
 * client -- ID of client to receive from.
 * msg -- pre-allocated buffer to hold message.
 * len -- number of bytes of message to read.
 * Returns: number of bytes received.
 * Note: msg buffer must have at least len bytes allocated.
 */
ssize_t cfroms(void *msg, size_t len)
{
	ssize_t n;
	
	if ((n = read(sfd, msg, len)) < 0)
	{
		perror("read error");
		exit(-1);
	}
	return n;
}


/*
 * Stop server-side ipc.
 * Note: must be called after sstart() to clean up.
 */
void sstop(void)
{
	int i;
	
	/* Close connections to clients */
	for (i = 0; i < nclient; i++)
	{
		close(cfd[i]);
	}
	free(cfd);
	
	/* Close master socket */
	close(sfd);
	unlink(path);
}


/*
 * Stop client-side ipc.
 * Note: must be called after cstart() to clean up.
 */
void cstop(void)
{
	close(sfd);
}