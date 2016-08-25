#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <limits.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>

#include "ulog.h"


//global variable
int debug = 0;


void mlogd_usage(char *prog_name)
{

#define USAGE_CONTENT \
	"Usage: %s [OPTIONS]\n" \
	"OPTIONS:\n" \
	"	-o <file>        # log filename\n" \
	"	-r <num>         # N-rotate log files to keep (default: %d)\n" \
	"	-s <size>        # max size of a log file in KB (default: %d)\n" \
	"	-t               # with timestamp\n" \
	"	-z <compressor>  # compress rotated file\n" \
	"	-d               # debug verbose\n" \
	""

	printf(USAGE_CONTENT, prog_name, DEFAULT_ROTATE, DEFAULT_LOGSIZE_KB);
}




int file_size(char *filename)
{
	struct stat st;

	if(stat(filename, &st) != 0) {
		return -1; //invalid file
	}
	return st.st_size;
}

int file_exist(char *filename)
{
	if(file_size(filename) >= 0) {
		return 1; //exist
	}

	return 0; //not exist
}


int shell_move(const char *src_file, const char *dst_file)
{
	char command[_POSIX_ARG_MAX] = {0};
	snprintf(command, sizeof(command), "mv -f -v %s %s >/dev/null 2>&1", src_file, dst_file);
	if(debug) {
		fprintf(stderr, "%s\n", command);
	}
	return system(command);
}

int shell_mkdir(char *path)
{
	char command[_POSIX_ARG_MAX] = {0};
	snprintf(command, sizeof(command), "mkdir -p %s", path);
	if(debug) {
		fprintf(stderr, "%s\n", command);
	}
	return system(command);
}

int shell_gzip(char *filename, char *suffix)
{
	char command[_POSIX_ARG_MAX] = {0};
	snprintf(command, sizeof(command), "gzip -f %s", filename);
	if(debug) {
		fprintf(stderr, "%s\n", command);
	}
	return system(command);
}

int shell_lzo(char *filename, char *suffix)
{
	char command[_POSIX_ARG_MAX] = {0};
	snprintf(command, sizeof(command), "lzop %s", filename);
	if(debug) {
		fprintf(stderr, "%s\n", command);
	}
	return system(command);
}

int shell_cat(char *filename)
{
	char command[_POSIX_ARG_MAX] = {0};
	snprintf(command, sizeof(command), "cat %s", filename);
	if(debug) {
		fprintf(stderr, "%s\n", command);
	}
	return system(command);
}

int shell_zcat(char *filename)
{
	char command[_POSIX_ARG_MAX] = {0};
	snprintf(command, sizeof(command), "zcat %s", filename);
	if(debug) {
		fprintf(stderr, "%s\n", command);
	}
	return system(command);
}

int shell_lzocat(char *filename)
{
	char command[_POSIX_ARG_MAX] = {0};
	snprintf(command, sizeof(command), "lzop -d < %s", filename);
	if(debug) {
		fprintf(stderr, "%s\n", command);
	}
	return system(command);
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


int rotate_files(struct config *conf, int new_session)
{
	int index = 0;
	char src_filename[MAX_FILENAME_SIZE] = {0};
	char dst_filename[MAX_FILENAME_SIZE] = {0};

	shell_mkdir(dirname((char *)format_string(src_filename, sizeof(src_filename), "%s", conf->log_name)));

	if(file_size(conf->log_name) > 0) {
		for(index = conf->max_rotate - 1; index >= 0; index--) {
			format_string(src_filename, sizeof(src_filename), "%s.%d%s", conf->log_name, index, compress_suffix[conf->compress]);
			if(file_exist(src_filename) > 0) {
				format_string(dst_filename, sizeof(dst_filename), "%s.%d%s", conf->log_name, index + 1, compress_suffix[conf->compress]);
				shell_move(src_filename, dst_filename);
			}
		}
		//last one, do gzip compress and move
		if(conf->compress == COMPRESS_LZO) {
			shell_lzo(conf->log_name, compress_suffix[conf->compress]);
		} else {
			shell_gzip(conf->log_name, compress_suffix[conf->compress]);
		}
		format_string(src_filename, sizeof(src_filename), "%s%s", conf->log_name, compress_suffix[conf->compress]);
		format_string(dst_filename, sizeof(dst_filename), "%s.%d%s", conf->log_name, 0, compress_suffix[conf->compress]);
		shell_move(src_filename, dst_filename);
	} else {
		//just remove empty or invalid file
		unlink(conf->log_name);
	}


	return 0;
}

const char *get_uptime()
{
	static FILE *fp = NULL;
	static char str_uptime[64] = {0};

	if(fp == NULL) {
		fp = fopen("/proc/uptime", "r");
		setvbuf(fp, NULL, _IONBF, 0);
		if(fp == NULL) {
			return "invalid_uptime";
		}
	}

	rewind(fp);
	fgets(str_uptime, sizeof(str_uptime), fp);

	char *next = NULL;
	return strtok_r(str_uptime, " ", &next);
}


int mlogd_READ_MODE_LINE(struct config *conf)
{
	char *line = NULL;
	size_t line_size = 0;

	//always rotate: prevent the new log append to old one.
	rotate_files(conf, 1);

	FILE *fp = fopen(conf->log_name, "w");
	setlinebuf(fp);

	size_t log_size = 0;
	size_t read = 0;

	while((read = getline(&line, &line_size, stdin)) != -1) {
		if(conf->timestamp == TIMESTAMP_UPTIME) {
			log_size += fprintf(fp, "[%12s] ", get_uptime());
		}

		fwrite(line, read, 1, fp);
		log_size += read;

		if(log_size >= conf->log_size) {
			fclose(fp);
			rotate_files(conf, 0);
			fp = fopen(conf->log_name, "w");	//reset fp
			setlinebuf(fp);
			log_size = 0;
		}
	}

	free(line);
	return 0;
}

int mlogd_READ_MODE_BINARY(struct config *conf)
{
	char buff[MAX_LINE_SIZE];
	size_t buff_size = MAX_LINE_SIZE;

	//always rotate: prevent the new log append to old one.
	rotate_files(conf, 1);

	FILE *fp = fopen(conf->log_name, "w");
	setvbuf(fp, NULL, _IONBF, 0);

	size_t log_size = 0;
	size_t read = 0;

	while(feof(fp) == 0 && ferror(fp)  == 0) {
		read = fread(buff, 1, buff_size, fp);

		fwrite(buff, read, 1, fp);
		log_size += read;

		if(log_size >= conf->log_size) {
			fclose(fp);
			rotate_files(conf, 0);
			fp = fopen(conf->log_name, "w");	//reset fp
			setvbuf(fp, NULL, _IONBF, 0);
			log_size = 0;
		}
	}

	return 0;
}

int mlogd(int argc, char *argv[])
{
	struct config default_config = {
		.read_mode = READ_MODE_LINE,
		.log_name = {0},
		.max_rotate = DEFAULT_ROTATE,
		.log_size = DEFAULT_LOGSIZE_KB * 1024,
		.timestamp = TIMESTAMP_NONE,
		.compress = COMPRESS_NONE,
	};

	int opt;
	while((opt = getopt(argc, argv, "do:r:s:tz:")) != -1) {
		switch(opt) {
		case 'd':
			debug = 1;
			break;
		case 'o':
			snprintf(default_config.log_name, sizeof(default_config.log_name), "%s", optarg);
			break;
		case 'r':
			default_config.max_rotate = strtoul(optarg, NULL, 10);
			break;
		case 's':
			default_config.log_size = strtoul(optarg, NULL, 10) * 1024;
			break;
		case 't':
			default_config.timestamp = TIMESTAMP_UPTIME;
			break;
		case 'z':
			if(strcmp(optarg, "gzip") == 0) {
				default_config.compress = COMPRESS_GZIP;
			} else if(strcmp(optarg, "lzo") == 0) {
				default_config.compress = COMPRESS_LZO;
			} else {
				default_config.compress = COMPRESS_NONE;
			}
			break;
		default:
			mlogd_usage(argv[0]);
			exit(1);
		}
	}

	if(strlen(default_config.log_name) == 0) {
		fprintf(stderr, "Error: missing config log filename: -o <file>\n");
		exit(1);
	}

	if(default_config.read_mode == READ_MODE_LINE) {
		return mlogd_READ_MODE_LINE(&default_config);
	} else if(default_config.read_mode == READ_MODE_BINARY) {
		return mlogd_READ_MODE_BINARY(&default_config);
	} else {
		fprintf(stderr, "Error: not suppport mode: %d", default_config.read_mode);
		return -1;
	}

	return 0;
}


int mlogread(int argc, char *argv[])
{
	struct config default_config = {
		.read_mode = READ_MODE_LINE,
		.log_name = {0},
		.max_rotate = DEFAULT_ROTATE,
		.log_size = DEFAULT_LOGSIZE_KB * 1024,
		.timestamp = TIMESTAMP_NONE,
		.compress = COMPRESS_NONE,
	};

	int opt;
	while((opt = getopt(argc, argv, "do:r:s:tz:")) != -1) {
		switch(opt) {
		case 'd':
			debug = 1;
			break;
		case 'o':
			snprintf(default_config.log_name, sizeof(default_config.log_name), "%s", optarg);
			break;
		case 'r':
			default_config.max_rotate = strtoul(optarg, NULL, 10);
			break;
		case 's':
			default_config.log_size = strtoul(optarg, NULL, 10) * 1024;
			break;
		case 't':
			default_config.timestamp = TIMESTAMP_UPTIME;
			break;
		case 'z':
			if(strcmp(optarg, "gzip") == 0) {
				default_config.compress = COMPRESS_GZIP;
			} else if(strcmp(optarg, "lzo") == 0) {
				default_config.compress = COMPRESS_LZO;
			} else {
				default_config.compress = COMPRESS_NONE;
			}
			break;
		default:
			mlogd_usage(argv[0]);
			exit(1);
		}
	}

	struct config *conf = &default_config;

	int index = 0;
	char filename[MAX_FILENAME_SIZE] = {0};
	for(index = conf->max_rotate - 1; index >= 0; index--) {
		format_string(filename, sizeof(filename), "%s.%d%s", conf->log_name, index, compress_suffix[conf->compress]);
		if(file_exist(filename)) {
			compress_cat[conf->compress](filename);
		}
	}

	if(file_exist(conf->log_name)) {
		shell_cat(conf->log_name);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	if(strcmp(basename(argv[0]), "ulog") == 0) {
		return mlogd(argc, argv);
	} else if(strcmp(basename(argv[0]), "ulogread") == 0) {
		return mlogread(argc, argv);
	}

	return -1;
}
