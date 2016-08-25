#ifndef _ULOG_H
#define _ULOG_H

#include <stdlib.h>

//default parameter
#define DEFAULT_ROTATE		5
#define DEFAULT_LOGSIZE_KB	64 //KB

#define MAX_FILENAME_SIZE	512
#define MAX_LINE_SIZE		1024
#define MAX_TIMESTAMP_SIZE	32

enum {
	READ_MODE_LINE,
	READ_MODE_BINARY,
};

enum {
	TIMESTAMP_NONE,
	TIMESTAMP_UPTIME,
	TIMESTAMP_FMT,
};

#define COMPRESS_TABLE 							\
	X(COMPRESS_NONE, 	"",		shell_cat)		\
	X(COMPRESS_GZIP,	".gz",	shell_zcat)		\
	X(COMPRESS_LZO,		".lzo",	shell_lzocat)	\

enum {
#define X(name, suffix, cat)	name,
	COMPRESS_TABLE
#undef X
};

static char *const compress_suffix[] = {
#define X(name, suffix, cat)	suffix,
	COMPRESS_TABLE
#undef X
};



struct config {
	char			log_name[MAX_FILENAME_SIZE];
	int 			read_mode;
	int				max_rotate;
	unsigned int	log_size;
	unsigned int	total_log_size;

	unsigned int		timestamp;
	char	timsstamp_format[MAX_TIMESTAMP_SIZE]; //used in strftime()

	unsigned int	compress;
};

int shell_cat(char *filename);
int shell_zcat(char *filename);
int shell_lzocat(char *filename);


static int (*compress_cat[])(char *) = {
#define X(name, suffix, cat)	cat,
	COMPRESS_TABLE
#undef X
};

#endif //_ULOG_H
