#include <stdio.h>
#include <stdarg.h>
#include "error.h"

ehandler_ptr ehandler;
int einit = 0;

/*
 * Default error handler.
 * message -- error message to print to stderr.
 * Note: this should be replaced with a user-defined
 * alternative that performs proper clean-up.
 */
static void default_handler(const char *message)
{
	perror(message);
	exit(-1);
}

/*
 * Initialise error handler.
 * handler -- function to handle errors.
 * Note: should be called before initialisation functions
 * from msg/draw.
 */
void estart(ehandler_ptr handler)
{
	if (!einit)
	{
		if (handler == NULL)
			ehandler = default_handler;
		else
			ehandler = handler;
		einit = 1;
	}
}

/*
 * Memory allocator that calls ehandler on fail.
 * len -- number of bytes to allocate.
 * Returns: pointer to allocated data.
 */
void* emalloc(size_t len)
{
	void *p;
	
	if ((p = malloc(len)) == NULL)
		ehandler("malloc error");
	return p;
}

/*
 * Print to stderr.
 */
int eprintf(const char *format, ...)
{
	int ret;
	va_list args;
	
	va_start(args, format);
	ret = vfprintf(stderr, format, args);
	va_end(args);
	return ret;
}
