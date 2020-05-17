#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include "draw.h"

int g(void)
{
	int x, y;
	char message[20];
	
	/* Clear the screen */
	clear();
	
	/* Draw a nice border */
	border('|', '|', '-', '-', '+', '+', '+', '+');
	
	/* Print message to random location on screen */
	strncpy(message, "Hello world!", sizeof(message));
	x = 1 + rand() % (winx - 2 - strlen(message));
	y = 1 + rand() % (winy - 2);
	mvprintw(y, x, message);
	
	return 0;
}

int f(int key)
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

int main(void)
{
	/* Get terminal ready for drawing */
	dstart();

	/* Game loop (minimum 500 ms per loop)  */
	dloop(500, &g, &f);
	
	/* Reset terminal for normal use */
	dstop();
	
	return 0;
}
