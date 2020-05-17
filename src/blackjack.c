#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <curses.h>
#include "draw.h"
#include "msg.h"

void usage(char *cmd)
{
	printf("usage: %s [-h nclient] path\n", cmd);
	exit(-1);
}

void deal(void)
{
	/* TODO */
}

int splayerg(void)
{
	/* TODO */
	return 0;
}

int sdealerg(void)
{
	/* TODO */
	return 0;
}

int splayerf(int key)
{
	/* TODO */
	return 0;
}

int sdealerf(int key)
{
	/* TODO */
	return 0;
}

int cplayerg(void)
{
	/* TODO */
	return 0;
}

int cplayerf(int key)
{
	/* TODO */
	return 0;
}

void servermain(int nclient, char *path)
{
	/* Initialisation */
	sstart(nclient, path);
	dstart();
	
	/* Deal out cards */
	deal();
	
	/* Process players' turns */
	dloop(500, splayerg, splayerf);
	
	/* Process dealer's turn */
	dloop(500, sdealerg, sdealerf);
	
	/* Clean-up */
	dstop();
	sstop();
}

void clientmain(char *path)
{
	/* Initialisation */
	cstart(path);
	dstart();
	
	/* Player's turn */
	dloop(500, cplayerg, cplayerf);
	
	/* Clean-up */
	dstop();
	cstop();
}

int main(int argc, char **argv)
{
	int c, hflag, nclient;
	
	hflag = nclient = 0;
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
