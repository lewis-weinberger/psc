#include <curses.h>
#include <locale.h>
#include <sys/time.h>
#include <time.h>
#include "draw.h"
#include "error.h"

int winx, winy;
int oldwinx, oldwiny;
int winchanged = 0;

/*
 * Initialise the terminal for drawing.
 * Note: must call dstop() to clean up when finished.
 */
void dstart(void)
{
	estart(NULL);
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	noecho();
	nodelay(stdscr, TRUE);
	keypad(stdscr, TRUE);
	curs_set(0);
	clear();
	getyx(stdscr, winy, winx);
}

/*
 * Game loop.
 * tick -- minimum tick time (in milliseconds) for each loop.
 * g -- pointer to function, to be run every loop iteration.
 * f -- pointer to function, to be run on user input keys.
 * Note: the functions g and f should return an integer > 0 whilst
 * the loop runs. The loop will exit if g or f returns < 0.
 * g is called before f.
 */
void dloop(long tick, int (*g)(void), int (*f)(int key))
{
	struct timeval start, end;
	struct timespec dt;
	int ch;
	
	for (;;)
	{
		/* Start time, millisecond resolution */
		gettimeofday(&start, NULL);
		
		/* Current window size */
		oldwinx = winx;
		oldwiny = winy;
		getmaxyx(stdscr, winy, winx);
		if (winx != oldwinx || winy != oldwiny)
			winchanged = 1;
		else
			winchanged = 0;
		

		if (g() < 0)
			break;

		if((ch = getch()) != ERR)
		{
			if (f(ch) < 0)
				break;
		}
		
		/* Flush draws to terminal screen */
		refresh();

		/* Sleep for remaining tick time */
		gettimeofday(&end, NULL);
		dt.tv_sec = (tick / K) - (end.tv_sec - start.tv_sec);
		dt.tv_nsec = (tick % K) * M - (end.tv_usec - start.tv_usec) * K;
		if (dt.tv_sec * G + dt.tv_nsec > 0)
			nanosleep(&dt, NULL);
	}
}

/*
 * Check if terminal has been resized between loops.
 */
int dresized(void)
{
	return winchanged;
}

/*
 * Reset the terminal.
 * Note: should be called to clean up after dstart().
 */
void dstop(void)
{
	endwin();
}
