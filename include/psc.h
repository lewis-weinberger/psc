#ifndef _PSC_H
#define _PSC_H

#include <stddef.h>

typedef int (*psc_loop)(void*, int, int, int);
typedef int (*psc_cmp)(void*, void*, size_t);

typedef enum
{
	PSC_OK,
	PSC_EINST,
	PSC_ELOG,
	PSC_EDRAW,
} psc_err;

void    psc_run(void*, size_t, psc_loop, psc_cmp, int);
psc_err psc_draw(int, int, char*);

#endif /* _PSC_H */