#ifndef _MSG_H
#define _MSG_H

#include <sys/types.h>

enum
{
	SUN_PATH_MAX = 108,             /* Socket path size limit on Linux */
};

void    sstart(int, char*);         /* Start server-side ipc */
void    cstart(char*);              /* Start client-side ipc */
ssize_t stoc(int, void*, size_t);   /* Send msg from server to client */
ssize_t ctos(void*, size_t);        /* Send msg from client to server */
ssize_t sfromc(int, void*, size_t); /* (Blocking) receive msg from client */
ssize_t cfroms(void*, size_t);      /* (Blocking) receive msg from server */
void    sstop(void);                /* Stop server-side ipc */
void    cstop(void);                /* Stop client-side ipc */

#endif /* _MSG_H */