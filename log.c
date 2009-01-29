#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "log.h"

/* TODO: Make the log file not stderr */

void log_init()
{
	/* TODO: Open a logfile */
}

void log_inform(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

void log_warn(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

void die(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	fprintf(stderr, "Fatal: ");
	vfprintf(stderr, fmt, args);
	va_end(args);

	abort();
}
