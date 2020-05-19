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
	TICK_TIME = 50,
	QUIT = -1,
};

/* Store game state in null-terminated strings */
struct game
{
	int  nplayers;
	int  current;
	int  finish;
	char names[PLAYERS_MAX][20];
	char hands[PLAYERS_MAX][CARDS_MAX+1];
	int  scores[PLAYERS_MAX];
	char deck[CARDS_MAX+1];
};

typedef struct game game;
game state;
int plid, uhit, ustand, previous, turn, finished;
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

void drawcard(int plid, int n)
{
	int c, b;
	
	c = strlen(state.deck);
	b = rand() % c;
	state.hands[plid][n] = state.deck[b];
	state.deck[b] = state.deck[c - 1];
	state.deck[c - 1] = '\0';
}

void deal(void)
{
	int i, j, b, pool[52];
	
	srand(time(0));
	
	/* Initialise game state */
	memset(state.hands, '\0', sizeof(state.hands));
	memset(state.deck, '\0', sizeof(state.deck));
	state.finish = state.current = turn = 0;
	
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
			drawcard(i, j);
		}
		state.scores[i] = score(state.hands[i]);
	}
	previous = strlen(state.deck);
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
	
	/* Draw title and info */
	if (state.current == plid)
		mvaddstr(1, 1, "BLACKJACK    > Your Turn! Press [H] to hit, or [S] to stick.");
	else
		mvaddstr(1, 1, "BLACKJACK");
		
	if (state.scores[plid] > 21)
		mvaddstr(1, 1, "BLACKJACK    > BUST!!!                                      ");
		
	/* Print the dealt hands */
	for (i = 0; i < state.nplayers; i++)
	{
		mvaddstr(4 + 4*i, 2, state.names[i]);
		if (i == state.nplayers - 1 && plid != state.nplayers - 1)
			snprintf(score, sizeof(score), "[?]");
		else
			snprintf(score, sizeof(score), "[%d]", state.scores[i]);
		mvaddstr(4 + 4*i, 2 + strlen(state.names[i]), score);
		for(j = 0; j < strlen(state.hands[i]); j++)
		{
			if (j == 0 && i == state.nplayers - 1 && plid != state.nplayers - 1)
				printcard(3 + 4*i, 7 + strlen(state.names[i]) + 5*j, ' ');
			else
				printcard(3 + 4*i, 7 + strlen(state.names[i]) + 5*j, state.hands[i][j]);
		}
	}
}

int dealer(void)
{
	int i, ready, hit;
	
	if (dresized() || strlen(state.deck) != previous || state.current != turn)
		printdeal();
		
	if (finished)
		return 0;

	/* Send game state to players */
	previous = strlen(state.deck);
	turn = state.current;
	for (i = 0; i < state.nplayers - 1; i++)
		stoc(i, &state, sizeof(state));
		
	if (state.finish)
	{
		finished = 1;
		return 0;
	}

	if (state.current == plid) /* Process dealer's turn */
	{
		/* Check if bust */
		if (state.scores[plid] > 21)
		{
			ustand = 1;
			uhit = 0;
		}
		
		if (uhit)
		{
			/* Draw new card */
			drawcard(state.current, strlen(state.hands[state.current]));
			state.scores[state.current] = score(state.hands[state.current]);
		}
		else if (ustand)
			state.finish = 1;
	}
	else
	{
		/* Check if current player is ready */
		sfromc(state.current, &ready, sizeof(ready));
		if (ready)
		{
			/* Check if they hit or stand */
			sfromc(state.current, &hit, sizeof(hit));
			if (hit)
			{
				/* Draw new card */
				drawcard(state.current, strlen(state.hands[state.current]));
				state.scores[state.current] = score(state.hands[state.current]);
			}
			else
				state.current++;
		}
	}
	return 0;
}

int player(void)
{
	int ready, z;
	
	if (dresized() || strlen(state.deck) != previous || state.current != turn)
		printdeal();
		
	if (finished)
		return 0;
		
	/* Receive current game state */
	previous = strlen(state.deck);
	turn = state.current;
	cfroms(&state, sizeof(state));
	
	if (state.finish)
	{
		finished = 1;
		return 0;
	}
	
	/* Check if it's our turn */
	if (state.current == plid)
	{
		/* Check if bust */
		if (state.scores[plid] > 21)
		{
			ustand = 1;
			uhit = 0;
		}
			
		/* Let server know when ready */
		ready = uhit || ustand;
		ctos(&ready, sizeof(ready));
	
		if (uhit) /* Hit for another card */
		{
			z = 1;
			ctos(&z, sizeof(z));
			uhit = 0;
		}
		else if (ustand) /* Stand with current hand */
		{
			z = 0;
			ctos(&z, sizeof(z));
			ustand = 0;
		}
	}

	return 0;
}

int input(int key)
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
		sfromc(i, state.names[i], sizeof(state.names[i]));

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
	
	/* Process turns */
	dloop(TICK_TIME, dealer, input);
	
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
	dloop(TICK_TIME, player, input);
	
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
	
	hflag = nclient = ustand = uhit = finished = 0;
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
