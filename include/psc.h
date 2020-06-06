#ifndef _PSC_H
#define _PSC_H

#include <stddef.h>
#include <curses.h>

#define FALSE 0
#define TRUE 1
#define PSC_UP KEY_UP
#define PSC_DOWN KEY_DOWN
#define PSC_LEFT KEY_LEFT
#define PSC_RIGHT KEY_RIGHT
#define PSC_BACKSPACE KEY_BACKSPACE
#define PSC_DELETE KEY_DC

typedef int (*psc_loop)(void*, int, int, int);

void psc_run(void*, size_t, psc_loop, int);
int  psc_draw(int, int, int, char[]);

#endif /* _PSC_H */