#ifndef _PSC_H
#define _PSC_H

#include <stddef.h>

#define FALSE 0
#define TRUE 1

typedef int (*psc_init)(void);
typedef int (*psc_loop)(void*, int, int);
typedef void (*psc_fin)(void);

void psc_run(void*, size_t, psc_init, psc_loop, psc_fin, int, int);

#endif /* _PSC_H */

