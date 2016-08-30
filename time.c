#include <stdio.h>
#include <stdlib.h>
#include <string.h>


const char *get_uptime_str()
{
	static FILE *fp = NULL;
	static char str_uptime[64] = {0};

	if(fp == NULL) {
		fp = fopen("/proc/uptime", "r");
		if(fp == NULL) {
			return "invalid_uptime";
		}
		//setvbuf(fp, NULL, _IONBF, 0);
	}

	rewind(fp);
	fgets(str_uptime, sizeof(str_uptime), fp);

	char *next = NULL;
	return strtok_r(str_uptime, " ", &next);
}
