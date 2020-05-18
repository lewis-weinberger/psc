#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <curses.h>
#include "draw.h"
#include "msg.h"
#include "error.h"

enum
{
	PLAYERS_MAX = 8,
	CARDS_MAX = 52,
};

/* Store game state in null-terminated strings */
struct game
{
	int nplayers;
	char hands[PLAYERS_MAX][CARDS_MAX+1];
	char deck[CARDS_MAX+1];
};
typedef struct game game;

int plid;
game state;
const char cards[52] = { 'A', 'A', 'A', 'A',
                         '2', '2', '2', '2',
                         '3', '3', '3', '3',
                         '4', '4', '4', '4',
                         '5', '5', '5', '5',
                         '6', '6', '6', '6',
                         '7', '7', '7', '7',
                         '8', '8', '8', '8',
                         '9', '9', '9', '9',
                         'X', 'X', 'X', 'X',
                         'J', 'J', 'J', 'J',
                         'Q', 'Q', 'Q', 'Q',
                         'K', 'K', 'K', 'K'};


void deal(void)
{
	int i, j, b, c, pool[52];
	
	/* Initialise game state */
	memset(state.hands, '\0', sizeof(state.hands));
	memset(state.deck, '\0', sizeof(state.deck));
	
	/* Shuffle deck */
	for (i = 0; i < 52; i++)
	{
		b = 1;
		while (b)
		{
			b = 0;
			pool[i] = rand() % 52;
			for (j = 0; j < i; j++)
			{
				if (pool[i] == pool[j])
					b = 1;
			}
		}
		state.deck[i] = cards[pool[i]];
	}
	
	/* Deal cards */
	for (i = 0; i < state.nplayers; i++)
	{
		for (j = 0; j < 2; j++)
		{
			c = strlen(state.deck);
			b = rand() % c;
			state.hands[i][j] = state.deck[b];
			state.deck[b] = state.deck[c - 1];
			state.deck[c - 1] = '\0';
		}
	}
}

void printcard(int y, int x, char card)
{
	if (card == 'X')
	{
		mvprintw(y, x, "+--+");
		mvprintw(y + 1, x, "|10|");
		mvprintw(y + 2, x, "+--+");
	}
	else
	{
		mvprintw(y, x, "+-+");
		mvprintw(y + 1, x, "|%c|", card);
		mvprintw(y + 2, x, "+-+");
	}
}

void printdeal(void)
{
	int i, j;
	
	/* Clear the screen */
	clear();
	
	/* Draw a nice border */
	border('|', '|', '-', '-', '+', '+', '+', '+');
	
	/* Draw a nice title */
	
	/* Print the dealt hands */
	for (i = 0; i < state.nplayers; i++)
	{
		if (i == plid)
			mvprintw(2 + 4*i, 1, "--> ");
		for(j = 0; j < strlen(state.hands[i]); j++)
			printcard(1 + 4*i, 5 + 4*j, state.hands[i][j]);
	}
}

int splayerg(void)
{
	return 0;
}

int sdealerg(void)
{
	return 0;
}

int splayerf(int key)
{
	/* Process user input */
	switch (key)
	{
		case 'q':
		case 'Q':
			/* q/Q quits the loop */
			return -1;
		default:
			break;
	}
	return 0;
}

int sdealerf(int key)
{
	/* Process user input */
	switch (key)
	{
		case 'q':
		case 'Q':
			/* q/Q quits the loop */
			return -1;
		default:
			break;
	}
	return 0;
}

int cplayerg(void)
{
	return 0;
}

int cplayerf(int key)
{
	/* Process user input */
	switch (key)
	{
		case 'q':
		case 'Q':
			/* q/Q quits the loop */
			return -1;
		default:
			break;
	}
	return 0;
}

void sehandler(const char *message)
{
	dstop();
	sstop();
	perror(message);
	exit(-1);
}

void cehandler(const char *message)
{
	dstop();
	cstop();
	perror(message);
	exit(-1);
}

void servermain(int nclient, char *path)
{
	int i;
	
	/* Initialisation */
	estart(sehandler);
	sstart(nclient, path);
	dstart();
	
	/* Deal out cards */
	deal();
	
	/* Send initial game state to other players */
	for (i = 0; i < nclient; i++)
	{
		stoc(i, &i, sizeof(int));
		stoc(i, &state, sizeof(state));
	}
	
	/* Draw dealt hands */
	printdeal();
	
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
	estart(cehandler);
	cstart(path);
	dstart();
	
	/* Receive initial game state */
	cfroms(&plid, sizeof(int));
	cfroms(&state, sizeof(state));
	
	/* Draw dealt hands */
	printdeal();
	
	/* Player's turn */
	dloop(500, cplayerg, cplayerf);
	
	/* Clean-up */
	dstop();
	cstop();
}

void usage(char *cmd)
{
	printf("usage: %s [-h nclient] path\n", cmd);
	exit(-1);
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
	{
		if (nclient + 1 > PLAYERS_MAX)
		{
			printf("%s only supports %d players!\n", argv[0], PLAYERS_MAX);
			usage(argv[0]);
		}
		state.nplayers = nclient + 1;
		plid = nclient;
		servermain(nclient, argv[optind]);
	}
	else
		clientmain(argv[optind]);
	
	return 0;
}
