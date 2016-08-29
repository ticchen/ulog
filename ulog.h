#ifndef _ULOG_H
#define _ULOG_H

#include <stdlib.h>

//default parameter
#define DEFAULT_ROTATE		5
#define DEFAULT_LOG_SIZE	64*1024
#define DEFAULT_COMPRESS_TYPE

#define MAX_FILENAME_SIZE	512
#define MAX_LINE_SIZE		1024
#define MAX_TIMESTAMP_SIZE	32
#define MAX_ARGV_SIZE		20

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
	X(COMPRESS_NONE,	"none", "",		shell_cat)		\
	X(COMPRESS_GZIP,	"gzip",	".gz",	shell_zcat)		\
	X(COMPRESS_LZO,		"lzo",	".lzo",	shell_lzocat)	\

enum {
#define X(type, name, suffix, cat)	type,
	COMPRESS_TABLE
#undef X
};

static int compress_type[] = {
#define X(type, name, suffix, cat)	type,
	COMPRESS_TABLE
#undef X
};


static char *const compress_name[] = {
#define X(type, name, suffix, cat)	name,
	COMPRESS_TABLE
#undef X
};

static char *const compress_suffix[] = {
#define X(type, name, suffix, cat)	suffix,
	COMPRESS_TABLE
#undef X
};

struct config {
	unsigned int	append;
	unsigned int	banner;
	char			config_file[MAX_FILENAME_SIZE];
	unsigned int	debug_level;
	char			log_file[MAX_FILENAME_SIZE];
	//int 			read_mode;
	int				rotate;
	/* log size */
	unsigned int	log_size;
	unsigned int	total_log_size;
	/* timestamp */
	unsigned int	timestamp_type;
	char			timestamp_format[MAX_TIMESTAMP_SIZE]; //used in strftime()
	/* compress type*/
	unsigned int	compress_type;
};

int shell_cat(char *filename);
int shell_zcat(char *filename);
int shell_lzocat(char *filename);

static int (*compress_cat[])(char *) = {
#define X(type, name, suffix, cat)	cat,
	COMPRESS_TABLE
#undef X
};


#endif //_ULOG_H
