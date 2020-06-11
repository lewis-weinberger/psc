#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <curses.h>
#include "psc.h"

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
	int  init;
	int  nplayers;
	int  current;
	int  finish;
	char names[PLAYERS_MAX][21];
	char hands[PLAYERS_MAX][CARDS_MAX+1];
	int  scores[PLAYERS_MAX];
	char deck[CARDS_MAX+1];
};

typedef struct game game;

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

int uhit, ustand, unew, uquit;
char name[21];

int score(char *hand, int ace)
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
				if (ace)
					ret += 11;
				else
					ret += 1;
				break;
			default:
				ret += hand[i] - '0';
		}
	}
	return ret;
}

void drawcard(game *state, int plid, int n)
{
	int c, b;

	c = strlen(state->deck);
	b = rand() % c;
	state->hands[plid][n] = state->deck[b];
	state->deck[b] = state->deck[c - 1];
	state->deck[c - 1] = '\0';
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

int win(game *state, int plid)
{
	int pscore_lo, pscore_hi, dscore_lo, dscore_hi;

	pscore_lo = state->scores[plid];
	pscore_hi = score(state->hands[plid], 1);
	dscore_lo = state->scores[state->nplayers - 1];
	dscore_hi = score(state->hands[state->nplayers - 1], 1);

	if (pscore_lo > 21) /* Player bust */
		return 0;
	else if (dscore_lo > 21) /* Dealer bust */
		return 1;

	if (pscore_lo > dscore_hi)
		return 1;
	else if (pscore_hi <= 21 && pscore_hi > dscore_hi)
		return 1;
	else if (pscore_lo == dscore_lo || pscore_hi == dscore_hi)
		return 2;
	else
		return 0;
}

void printdeal(game *state, int plid)
{
	int i, j, mywin;
	char score[5];

	/* Clear the screen */
	clear();

	/* Draw title and info */
	if (state->finish)
	{
		mvaddstr(1, 1, "BLACKJACK    > GAME COMPLETE... Thanks for playing! Press [Q] to quit.");
		if (plid != state->nplayers - 1)
		{
			mywin = win(state, plid);
			if (mywin == 1)
				mvaddstr(2, 1, "You beat the dealer!");
			else if (mywin == 2)
				mvaddstr(2, 1, "You drew with the dealer!");
			else
				mvaddstr(2, 1, "You lost to the dealer!");
		}
		else
		{
			mvaddstr(2, 1, "Press [N] to start a new game.");
		}
	}
	else if (state->current == plid)
		mvaddstr(1, 1, "BLACKJACK    > Your Turn! Press [H] to hit, or [S] to stick.");
	else if (state->scores[plid] > 21)
		mvaddstr(1, 1, "BLACKJACK    > BUST!!!");
	else
		mvaddstr(1, 1, "BLACKJACK");

	/* Print the dealt hands */
	for (i = 0; i < state->nplayers; i++)
	{
		mvaddstr(5 + 4*i, 2, state->names[i]);
		if (i == state->nplayers - 1 && !state->finish && plid != state->nplayers - 1)
			snprintf(score, sizeof(score), "[?]");
		else
			snprintf(score, sizeof(score), "[%d]", state->scores[i]);
		mvaddstr(5 + 4*i, 2 + strlen(state->names[i]), score);
		for(j = 0; j < strlen(state->hands[i]); j++)
		{
			if (j == 0 && i == state->nplayers - 1 && !state->finish && plid != state->nplayers - 1)
				printcard(4 + 4*i, 7 + strlen(state->names[i]) + 5*j, ' ');
			else
				printcard(4 + 4*i, 7 + strlen(state->names[i]) + 5*j, state->hands[i][j]);
		}
	}

	/* Draw a nice border */
	border('|', '|', '-', '-', '+', '+', '+', '+');

	refresh();
}

void deal(game *state)
{
	int i, j, b, pool[52];

	srand(time(0));

	/* Initialise game state */
	memset(state->hands, '\0', sizeof(state->hands));
	memset(state->deck, '\0', sizeof(state->deck));
	memset(state->names, '\0', sizeof(state->names));
	state->finish = FALSE;

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
		state->deck[i] = cards[pool[i]];
	}

	/* Deal cards */
	for (i = 0; i < state->nplayers; i++)
	{
		for (j = 0; j < 2; j++)
		{
			drawcard(state, i, j);
		}
		state->scores[i] = score(state->hands[i], 0);
	}
}


void input(void)
{
	uquit = unew = uhit = ustand = FALSE;
	/* Process user input */
	switch (getch())
	{
		case 'q':
		case 'Q':
			uquit = TRUE;
			break;
		case 'n':
		case 'N':
			unew = TRUE;
			break;
		case 'h':
		case 'H':
			uhit = TRUE;
			break;
		case 's':
		case 'S':
			ustand = TRUE;
			break;
		default:
			break;
	}
}

/* Initialise terminal screen */
int init(void)
{
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	noecho();
	nodelay(stdscr, TRUE);
	keypad(stdscr, TRUE);
	curs_set(0);
	clear();
	mvaddstr(0, 0, "Waitin for other players...");
	refresh();
	return 0;
}

/* Return terminal screen to normal state */
void fin(void)
{
	endwin();
}

/* Game loop */
int loop(void *in, int state_changed, int player_id)
{
	game *state;

	state = (game *)in;

	/* Initial game stage -- process name input */
	if (state->init)
	{
		if (state->current == player_id)
		{
			memcpy(state->names[state->current], name, sizeof(name));
			if (state->current < state->nplayers - 1)
				state->current++;
			else
			{
				state->current = 0;
				state->init = FALSE;
			}
		}
		return 0;
	}

	/* Print state if changed */
	if (state_changed)
		printdeal(state, player_id);

	/* User input */
	input();

	if (uquit)
	{
		state->finish = TRUE;
		return -1;
	}

	if (state->finish)
	{
		if (player_id == (state->nplayers - 1) && unew) /* Server can start a new game */
		{
			deal(state);
		}
		return 0;
	}

	/* Check if it's our turn */
	if (state->current == player_id)
	{
		/* Check if bust */
		if (state->scores[player_id] > 21)
		{
			ustand = TRUE;
			uhit = FALSE;
		}

		if (uhit) /* Hit for another card */
		{
			drawcard(state, state->current, strlen(state->hands[state->current]));
			state->scores[state->current] = score(state->hands[state->current], 0);
		}
		else if (ustand) /* Stand with current hand */
		{
			if (state->current < state->nplayers - 1)
				state->current++;
			else
			{
				state->current = 0;
				state->finish = TRUE;
			}
		}
	}

	return 0;
}

void usage(char *cmd)
{
	fprintf(stderr, "usage: %s [-h] [-n nclient]\n", cmd);
	exit(-1);
}

void help(char *cmd)
{
	printf("usage: %s [-h] [-n nclient]\n", cmd);
	printf("-h:\n\tprints this help\n");
	printf("-n:\n\tspecify number of clients (server only)\n");
	exit(0);
}

int main(int argc, char **argv)
{
	int c, hflag, nclient;
	game state;

	hflag = nclient = 0;
	while ((c = getopt(argc, argv, "hn:")) > 0)
	{
		switch(c)
		{
			case 'h':
				hflag = TRUE;
				break;
			case 'n':
				nclient = atoi(optarg);
				break;
			case '?':
				usage(argv[0]);
		}
	}

	if (hflag)
		help(argv[0]);

	printf("Please input your name [20 character limit]:\n");
	if (fgets(name, sizeof(name), stdin) == NULL)
	{
		perror("fgets error");
		exit(-1);
	}

	if (nclient > 0)
	{
		if (nclient + 1 > PLAYERS_MAX)
		{
			fprintf(stderr, "%s only supports %d players!\n", argv[0], PLAYERS_MAX);
			usage(argv[0]);
		}

		/* Setup game state */
		state.nplayers = nclient + 1;
		state.init = TRUE;
		state.current = 0;
		deal(&state);

		psc_run(&state, sizeof(state), &init, &loop, &fin,nclient);
	}
	else
	{
		memset(&state, 0, sizeof(state));
		psc_run(&state, sizeof(state), &init, &loop, &fin, 0);
	}

	return 0;
}
