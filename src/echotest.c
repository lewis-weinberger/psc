#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "msg.h"

void usage(char *cmd)
{
	printf("usage: %s [-h nclient] path\n", cmd);
	exit(-1);
}

void servermain(int nclient, char *path)
{
	int i;
	char msg[1024];
	
	/* Start server and connect to clients */
	sstart(nclient, path);
	
	/* Receive messages from connected clients */
	for (i = 0; i < nclient; i++)
	{
		sfromc(i, msg, 1024);
		printf("%s", msg);
	}
	
	/* Stop server */
	sstop();
}

void clientmain(char *path)
{
	char msg[1024];

	/* Start client and connect to server */
	cstart(path);
	
	/* Read a message from stdin to send to server */
	printf("Please enter a message to send to the server:\n");
	if (fgets(msg, sizeof(msg), stdin) == NULL)
	{
		perror("fgets error");
		exit(-1);
	}
	
	/* Send message to server */
	ctos(msg, 1024);
	
	/* Stop client */
	cstop();
}

int main(int argc, char **argv)
{
	int c, hflag, nclient;
	
	hflag = 0;
	while ((c = getopt(argc, argv, "h:")) > 0)
	{
		switch(c)
		{
			case 'h':
				nclient = atoi(optarg);
				hflag = 1;
				break;
			case '?':
				usage(argv[0]);
		}
	}
	if (optind >= argc)
		usage(argv[0]);
	
	if (hflag)
		servermain(nclient, argv[optind]);
	else
		clientmain(argv[optind]);
	
	return 0;
}
