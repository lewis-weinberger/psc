#ifndef _ERROR_H
#define _ERROR_H

#include <stdlib.h>

typedef void (*ehandler_ptr)(const char*);
extern ehandler_ptr ehandler;

void estart(ehandler_ptr);
void *emalloc(size_t);

#endif /* _ERROR_H */
