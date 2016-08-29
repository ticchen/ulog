#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

#include "common.h"

/********************************
 * Global variable
 ********************************/
static int _debug_level = 0;


/********************************
 * wifi debug level function
 ********************************/
void set_debug_level(int level)
{
	_debug_level = level;
}


int get_debug_level()
{
	return _debug_level;
}



/********************************
 * common helper function
 ********************************/
int do_system(char *format, ...)
{
	char command[_POSIX_ARG_MAX] = {0};

	va_list args;
	va_start(args, format);
	vsnprintf(command, sizeof(command), format, args);
	va_end(args);

	int ret = system(command);
	return ret;
}
