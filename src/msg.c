#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include "msg.h"
#include "error.h"

struct sockaddr_un addr;
int nclient, sfd, *cfd, reconflag;
char path[sizeof(addr.sun_path)];
fd_set readfds;
void (*reconnect)(int);

/*
 * Start the server-side ipc.
 * nc -- number of clients expected to join.
 * sp -- filepath of the (Unix domain) socket to create.
 * opt -- whether to attempt reconnect on error.
 * f -- function to call when attempting to reconnect.
 * Note: must call sstop() when finished, to clean up.
 */
void sstart(int nc, char *sp, int opt, void (*f)(int))
{
	int i;
	char welcome[1024];
	
	/* Initialise error handling */
	signal(SIGPIPE, SIG_IGN);
	estart(NULL);
	
	nclient = nc;
	if((reconflag = opt) == MSG_RECON && f)
		reconnect = f;
	strncpy(path, sp, sizeof(path) - 1);
	eprintf("Server starting, expecting %d client(s)\n", nclient);

	/* Allocate client sockets */
	cfd = emalloc(nclient * sizeof(int));
	memset(cfd, 0, nclient * sizeof(int));

	/* Create socket */
	if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		ehandler("socket error");

	/* Bind socket to path */
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, sizeof(addr.sun_path));
	unlink(path);
	if (bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		ehandler("bind error");
	eprintf("* Listening on socket: %s\n", path);

	/* Ensure permissions on socket allow (any) other users to join */
	if(chmod(path, S_IRWXG | S_IRWXU | S_IRWXO) < 0)
		ehandler("chmod error");

	/* Specify maximum number of connections */
	if (listen(sfd, nclient) < 0)
		ehandler("listen error");

	/* Wait for all connections */
	eprintf("* Waiting for client connections...\n");
	for (i = 0; i < nclient; i++)
	{
		/* Accept any incoming connection on master socket */
		if ((cfd[i] = accept(sfd, NULL, NULL)) < 0)
			ehandler("accept error");
		eprintf("* Client %d connected!\n", i);
		
		/* Send welcome message */
		strncpy(welcome, "--> [Server] Welcome!", sizeof(welcome));
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
	
	/* Initialise error handling */
	signal(SIGPIPE, SIG_IGN);
	estart(NULL);
	
	strncpy(path, sp, sizeof(path) - 1);
	eprintf("Client starting\n");

	/* Create socket */
	if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		ehandler("socket error");
	
	/* Connect to server */
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, sizeof(addr.sun_path));
	if (connect(sfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		ehandler("connect error");

	eprintf("* Connected to: %s\n", path);
	
	/* Read welcome message from server */
	cfroms(welcome, sizeof(welcome));
	eprintf("%s\n", welcome);
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
	char welcome[1024];
	
	if ((n = write(cfd[client], msg, len)) < 0)
	{
		if (errno == EPIPE && reconflag == MSG_RECON) /* No reader at other end of pipe */
		{
			close(cfd[client]);
			eprintf("Client connection closed, waiting for reconnection...\n");
			
			/* Wait for reconnection */
			if ((cfd[client] = accept(sfd, NULL, NULL)) < 0)
				ehandler("accept error");
			eprintf("* Client %d reconnected!\n", client);
			
			/* Send welcome message */
			strncpy(welcome, "--> [Server] Welcome!", sizeof(welcome));
			stoc(client, welcome, sizeof(welcome));
			
			/* User-defined reconnection procedure */
			reconnect(MSG_SEND);

			/* Continue as before, sending original message */
			stoc(client, msg, len);		
		}
		else if (errno == EINTR) /* Interrupted... try again */
			stoc(client, msg, len);
		else
			ehandler("read error");
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
		if (errno == EPIPE) /* No reader at other end of pipe */
			ehandler("Server closed");
		else if (errno == EINTR) /* Interrupted... try again */
			ctos(msg, len);
		else	
			ehandler("read error");
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
	char welcome[1024];
	
	if ((n = read(cfd[client], msg, len)) < 0)
	{
		if (errno == EINTR) /* Interrupted... try again */
			sfromc(client, msg, len);
		else
			ehandler("read error");
	}
	
	if (n == 0 && reconflag == MSG_RECON) /* No writer at other end of pipe */
	{
		close(cfd[client]);
		eprintf("Client connection closed, waiting for reconnection...\n");
		
		/* Wait for reconnection */
		if ((cfd[client] = accept(sfd, NULL, NULL)) < 0)
			ehandler("accept error");
		eprintf("* Client %d reconnected!\n", client);
		
		/* Send welcome message */
		strncpy(welcome, "--> [Server] Welcome!", sizeof(welcome));
		stoc(client, welcome, sizeof(welcome));

		/* User-defined reconnection procedure */
		reconnect(MSG_RECV);

		/* Continue as before, receiving message */
		memset(msg, 0, len);
		sfromc(client, msg, len);
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
		if (errno = EINTR) /* Interrupted... try again */
			cfroms(msg, len);
		else
			ehandler("read error");
	}
	
	if (n == 0) /* Socket closed at server end */
		ehandler("Server closed");
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
		close(cfd[i]);
	free(cfd);
	
	/* Close master socket */
	close(sfd);
	unlink(path);
	eprintf("Server stopped.\n");
}


/*
 * Stop client-side ipc.
 * Note: must be called after cstart() to clean up.
 */
void cstop(void)
{
	close(sfd);
	eprintf("Client stopped.\n");
}
