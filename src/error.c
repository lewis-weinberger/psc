#include <stdio.h>
#include "error.h"

/*
 *
 */
void* emalloc(size_t len)
{
	void *p;
	
	if ((p = malloc(len)) == NULL)
	{
		perror("malloc error");
		exit(-1);
	}
	return p;
}
