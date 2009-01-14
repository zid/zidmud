#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "log.h"

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
