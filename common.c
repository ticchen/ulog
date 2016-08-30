#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

#include "common.h"

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


const char *format_string(const char *str, size_t str_size, const char *format, ...)
{
	if(str == NULL || str_size == 0 || format == NULL) {
		return ""; //safe for stupid
	}

	va_list args;
	va_start(args, format);
	int len = vsnprintf((char *)str, str_size, format, args);
	va_end(args);
	if(len < 0 || str_size <= len) {
		fprintf(stderr, "Error: str is truncated");
	}
	return str;
}
