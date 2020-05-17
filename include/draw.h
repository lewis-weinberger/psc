#ifndef _DRAW_H
#define _DRAW_H

enum
{
	K = 1000,
	M = 1000000,
	G = 1000000000,
};

extern int winx, winy;

void dstart(void);   /* Start terminal drawing */
void dstop(void);    /* Stop terminal drawing */
void dloop(long, int (*g)(void), int (*f)(int)); /* Game loop */

#endif /* _DRAW_H */
