#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <curses.h>
#include "draw.h"
#include "msg.h"
#include "error.h"

enum
{
	PLAYERS_MAX = 8,
	CARDS_MAX = 52,
	TICK_TIME = 100,
	HIT = 1,
	STAND = 2,
	QUIT = -1,
};

/* Store game state in null-terminated strings */
struct game
{
	int  nplayers;
	char names[PLAYERS_MAX][20];
	char hands[PLAYERS_MAX][CARDS_MAX+1];
	int  scores[PLAYERS_MAX];
	char deck[CARDS_MAX+1];
};
typedef struct game game;

int plid, ustand, uhit;
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

int score(char *hand)
{
	int i, ret;
	
	ret = 0;
	for (i = 0; i < strlen(hand); i++)
	{
		switch (hand[i])
		{
			case 'X':
			case 'J':
			case 'Q':
			case 'K':
				ret += 10;
				break;
			case 'A':
				ret += 1;
				break;
			default:
				ret += hand[i] - '0';
		}
	}
	return ret;
}

void deal(void)
{
	int i, j, b, c, pool[52];
	
	srand(time(0));
	
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
		state.scores[i] = score(state.hands[i]);
	}
}

void printcard(int y, int x, char card)
{
	if (card == 'X')
	{
		mvaddstr(y, x, "+--+");
		mvaddstr(y + 1, x, "|10|");
		mvaddstr(y + 2, x, "+--+");
	}
	else
	{
		mvaddstr(y, x, "+-+");
		mvaddstr(y + 1, x, "|");
		mvaddch(y + 1, x + 1, card);
		mvaddstr(y + 1, x + 2, "|");
		mvaddstr(y + 2, x, "+-+");
	}
}

void printdeal(void)
{
	int i, j;
	char score[5];
	
	/* Clear the screen */
	clear();
	
	/* Draw a nice border */
	border('|', '|', '-', '-', '+', '+', '+', '+');
	
	/* Print the dealt hands */
	for (i = 0; i < state.nplayers; i++)
	{
		mvaddstr(2 + 4*i, 2, state.names[i]);
		if (i == state.nplayers - 1 && plid != state.nplayers - 1)
			snprintf(score, sizeof(score), "[?]");
		else
			snprintf(score, sizeof(score), "[%d]", state.scores[i]);
		mvaddstr(2 + 4*i, 2 + strlen(state.names[i]), score);
		for(j = 0; j < strlen(state.hands[i]); j++)
		{
			if (j == 0 && i == state.nplayers - 1 && plid != state.nplayers - 1)
				printcard(1 + 4*i, 7 + strlen(state.names[i]) + 5*j, ' ');
			else
				printcard(1 + 4*i, 7 + strlen(state.names[i]) + 5*j, state.hands[i][j]);
		}
	}
}

int splayerg(void)
{
	if (dresized())
		printdeal();
	return 0;
}

int sdealerg(void)
{
	int i, hit, c, b, j;
	
	if (dresized())
		printdeal();
		
	/* Receive player decisions */
	for (i = 0; i < state.nplayers - 1; i++)
	{
		sfromc(i, &hit, sizeof(hit));
		stoc(i, &state, sizeof(state));
		while (hit)
		{
			/* Draw new card */
			j = strlen(state.hands[i]);
			c = strlen(state.deck);
			b = rand() % c;
			state.hands[i][j] = state.deck[b];
			state.deck[b] = state.deck[c - 1];
			state.deck[c - 1] = '\0';
			state.scores[i] = score(state.hands[i]);
			
			/* Send state back to player */
			stoc(i, &state, sizeof(state));
			
			/* Wait for next move */
			if (state.scores[i] <= 21)
				sfromc(i, &hit, sizeof(hit));
			else
				hit = 0;
		}
	}
	return 0;
}

int splayerf(int key)
{
	/* Process user input */
	switch (key)
	{
		case 'q':
		case 'Q':
			return QUIT;
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
			return QUIT;
		case 'h':
		case 'H':
			uhit = 1;
			break;
		case 's':
		case 'S':
			ustand = 1;
			break;
		default:
			break;
	}
	return 0;
}

int cplayerg(void)
{
	int a, b;
	a = 1;
	b = 0;
	
	if (dresized())
		printdeal();
		
	if (uhit && state.scores[plid] <= 21)
	{
		ctos(&a, sizeof(int));
		cfroms(&state, sizeof(state));
		printdeal();
		uhit = 0;
	}
	else if (ustand)
	{
		ctos(&b, sizeof(int));
		cfroms(&state, sizeof(state));
		printdeal();
		ustand = 0;
	}

	return 0;
}

int cplayerf(int key)
{
	/* Process user input */
	switch (key)
	{
		case 'q':
		case 'Q':
			return QUIT;
		case 'h':
		case 'H':
			uhit = 1;
			break;
		case 's':
		case 'S':
			ustand = 1;
			break;
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
	printf("Please input your name [20 character limit]:\n");
	if (fgets(state.names[state.nplayers - 1], sizeof(state.names[0]), stdin) == NULL)
		ehandler("fgets error");
	sstart(nclient, path);
	
	/* Receive player names */
	for (i = 0; i < nclient; i++)
	{
		sfromc(i, state.names[i], sizeof(state.names[i]));
	}

	/* Deal out cards */
	deal();
	
	/* Send initial game state to other players */
	for (i = 0; i < nclient; i++)
	{
		stoc(i, &i, sizeof(int));
		stoc(i, &state, sizeof(state));
	}
	
	/* Draw dealt hands */
	dstart();
	printdeal();
	
	/* Process players' turns */
	dloop(TICK_TIME, splayerg, splayerf);
	
	/* Process dealer's turn */
	dloop(TICK_TIME, sdealerg, sdealerf);
	
	/* Clean-up */
	dstop();
	sstop();
}

void clientmain(char *path)
{
	char name[sizeof(state.names[0])];
	
	/* Initialisation */
	estart(cehandler);
	printf("Please input your name [20 character limit]:\n");
	if (fgets(name, sizeof(name), stdin) == NULL)
		ehandler("fgets error");
	cstart(path);
	
	/* Send name */
	ctos(name, sizeof(name));
	
	/* Receive initial game state */
	cfroms(&plid, sizeof(int));
	cfroms(&state, sizeof(state));
	
	/* Draw dealt hands */
	dstart();
	printdeal();
	
	/* Player's turn */
	dloop(TICK_TIME, cplayerg, cplayerf);
	
	/* Clean-up */
	dstop();
	cstop();
}

void usage(char *cmd)
{
	eprintf("usage: %s [-h nclient] path\n", cmd);
	exit(-1);
}

int main(int argc, char **argv)
{
	int c, hflag, nclient;
	
	hflag = nclient = ustand = uhit = 0;
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
			eprintf("%s only supports %d players!\n", argv[0], PLAYERS_MAX);
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
