#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <limits.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <getopt.h>

#include "ulog.h"
#include "common.h"
#include "file.h"
#include "time.h"


//global variable
int global_debug = 0;


void ulog_usage(char *prog_name)
{
	fprintf(stdout,
	        "Usage: %s [OPTIONS]\n"
	        "\n"
	        "Log standard input to rotate FILE\n"
	        "\n"
	        "	-a, --append                # append message to the given log, do not rotate\n"
	        "	-b, --banner                # when ulog start, log a banner message\n"
	        "	-c CONFIG, --config=CONFIG  # read setting from config\n"
	        "	-d, --debug                 # debug verbose\n"
	        "	-l FILE, --logfile=FILE     # log filename\n"
	        "   -m MODE, --mode=MODE        # work mode: \"log\":generate log, read:read log (default:log)\n"
	        "	-r N, --rotate=N            # N rotated logs to keep (default:%d, max=99, 0=purge)\n"
	        "	-s SIZE, --size=SIZE        # Max size (KB) before rotation (default:%d, 0=off)\n"
	        "	                            # support multiplicative suffixes: KB=1000, K=1024, MB=1000*1000, M=1024*1024\n"
	        "	-t, --timestamp             # log message with timestamp\n"
	        "	-z COMPRESSOR, --compress=C # compressors for rotated log: \"none\", \"gzip\". (default: none)\n"

	        "", prog_name, DEFAULT_ROTATE, DEFAULT_LOG_SIZE);
	return;
}

static struct option long_options[] = {
	{"append",		no_argument,		0,	'a'	},
	{"banner",		no_argument,		0,	'b'	},
	{"config",		required_argument,	0,	'c'	},
	{"debug",		no_argument,		0,	'd'	},
	{"logfile",		required_argument,	0,	'l'	},
	{"mode",		required_argument,	0,	'm'	},
	{"rotate", 		required_argument,	0,	'r'	},
	{"size",		required_argument,	0,	's'	},
	{"timestamp",	no_argument,		0,	't' },
	{"compress",	required_argument,	0,	'z' },
	{0, 0, 0, 0},
};




int shell_move(const char *src_file, const char *dst_file)
{
	return do_system("mv -f -v %s %s >/dev/null 2>&1", src_file, dst_file);
}

int shell_mkdir(char *path)
{
	return do_system("mkdir -p %s", path);
}

int shell_gzip(char *filename, char *suffix)
{
	return do_system("gzip -f %s", filename);
}

int shell_cat(char *filename)
{
	return do_system("cat %s", filename);
}

int shell_zcat(char *filename)
{
	return do_system("zcat %s", filename);
}


int rotate_files(struct config *conf, int new_session)
{
	int index = 0;
	char src_filename[MAX_FILENAME_SIZE] = {0};
	char dst_filename[MAX_FILENAME_SIZE] = {0};

	shell_mkdir(dirname((char *)format_string(src_filename, sizeof(src_filename), "%s", conf->log_file)));

	if(file_size(conf->log_file) > 0) {
		for(index = conf->rotate - 1; index >= 0; index--) {
			format_string(src_filename, sizeof(src_filename), "%s.%d%s", conf->log_file, index, compress_suffix[conf->compress_type]);
			if(file_exist(src_filename) > 0) {
				format_string(dst_filename, sizeof(dst_filename), "%s.%d%s", conf->log_file, index + 1, compress_suffix[conf->compress_type]);
				shell_move(src_filename, dst_filename);
			}
		}
		//last one, do gzip compress and move
		if(conf->compress_type == COMPRESS_GZIP) {
			shell_gzip(conf->log_file, compress_suffix[conf->compress_type]);
		}
		format_string(src_filename, sizeof(src_filename), "%s%s", conf->log_file, compress_suffix[conf->compress_type]);
		format_string(dst_filename, sizeof(dst_filename), "%s.%d%s", conf->log_file, 0, compress_suffix[conf->compress_type]);
		shell_move(src_filename, dst_filename);
	} else {
		//just remove empty or invalid file
		unlink(conf->log_file);
	}


	return 0;
}



int ulog_READ_MODE_LINE(struct config *conf)
{
	char *line = NULL;
	size_t line_size = 0;

	//always rotate: prevent the new log append to old one.
	rotate_files(conf, 1);

	FILE *fp = fopen(conf->log_file, "w");
	setlinebuf(fp);

	size_t log_size = 0;
	size_t read = 0;

	while((read = getline(&line, &line_size, stdin)) != -1) {
		if(conf->timestamp_type == TIMESTAMP_UPTIME) {
			log_size += fprintf(fp, "[%12s] ", get_uptime_str());
		}

		fwrite(line, read, 1, fp);
		log_size += read;

		if(log_size >= conf->log_size) {
			fclose(fp);
			rotate_files(conf, 0);
			fp = fopen(conf->log_file, "w");	//reset fp
			setlinebuf(fp);
			log_size = 0;
		}
	}

	free(line);
	return 0;
}

int ulog_READ_MODE_BINARY(struct config *conf)
{
	char buff[MAX_LINE_SIZE];
	size_t buff_size = MAX_LINE_SIZE;

	//always rotate: prevent the new log append to old one.
	rotate_files(conf, 1);

	FILE *fp = fopen(conf->log_file, "w");
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
			fp = fopen(conf->log_file, "w");	//reset fp
			setvbuf(fp, NULL, _IONBF, 0);
			log_size = 0;
		}
	}

	return 0;
}


int ulogread(struct config *conf)
{
	int index = 0;
	char filename[MAX_FILENAME_SIZE] = {0};
	for(index = conf->rotate - 1; index >= 0; index--) {
		format_string(filename, sizeof(filename), "%s.%d%s", conf->log_file, index, compress_suffix[conf->compress_type]);
		if(file_exist(filename)) {
			compress_cat[conf->compress_type](filename);
		}
	}

	if(file_exist(conf->log_file)) {
		shell_cat(conf->log_file);
	}
	return 0;
}

int ulog(struct config *conf)
{
	//check requirement
	if(strlen(conf->log_file) == 0) {
		return -1;
	}
	if(conf->mode == MODE_LOG_LINE) {
		return ulog_READ_MODE_LINE(conf);
	} else if(conf->mode == MODE_LOG_BINARY) {
		return ulog_READ_MODE_BINARY(conf);
	} else if(conf->mode == MODE_READ_LOG) {
		return ulogread(conf);
	}
	return -1;
}


int config_load_argument(struct config *conf, int argc, char *argv[])
{
	int val = 0;
	int option_index = 0;

	optind = 0; //reset opt index
	while(1) {
		val = getopt_long(argc, argv, "abc:dl:m:r:s:tz:", long_options, &option_index);
		if(val == -1) {
			break;
		}
		switch(val) {
		case 'a':
			conf->append = 1;
			break;
		case 'b':
			conf->banner = 1;
			break;
		case 'c':
			snprintf(conf->config_file, sizeof(conf->config_file), "%s", optarg);
			break;
		case 'd':
			conf->debug_level = 1;
			global_debug = conf->debug_level;
			break;
		case 'l':
			snprintf(conf->log_file, sizeof(conf->log_file), "%s", optarg);
			break;
		case 'm':
			if(strcmp(optarg, "log") == 0) {
				conf->mode = MODE_LOG_LINE;
			} else if(strcmp(optarg, "read") == 0) {
				conf->mode = MODE_READ_LOG;
			}
			break;
		case 'r':
			conf->rotate = strtoul(optarg, NULL, 10);
			break;
		case 's': {
			char *suffix = NULL;
			conf->log_size = strtoul(optarg, &suffix, 10);
			if(strcasecmp(suffix, "K") == 0 || strcasecmp(suffix, "KiB") == 0) {
				conf->log_size *= 1024;
			} else if(strcasecmp(suffix, "KB") == 0) {
				conf->log_size *= 1000;
			} else if(strcasecmp(suffix, "M") == 0 || strcasecmp(suffix, "MiB") == 0) {
				conf->log_size *= 1024 * 1024;
			} else if(strcasecmp(suffix, "MB") == 0) {
				conf->log_size *= 1000 * 1000;
			}
			break;
		}
		case 't':
			conf->timestamp_type = TIMESTAMP_UPTIME;
			break;
		case 'z': {
			int i = 0;
			for(i = 0; i < ARRAY_SIZE(compress_name); i++) {
				if(strcmp(compress_name[i], optarg) == 0) {
					conf->compress_type = compress_type[i];
					break;
				}
			}
			if(i == ARRAY_SIZE(compress_name)) { //run out
				fprintf(stderr, "Error: not support compressor: %s\n", optarg);
				return -1;
			}
			break;
		}
		default:
			return -1;
		}//end of switch
	}//end of while

	return 0;
}

int config_file_to_argv(char *config_file, char *argv[], unsigned int argv_size)
{
	int argc = 0;

	FILE *fp = fopen(config_file, "r");
	if(fp == NULL) {
		return 0; //no thing to parse
	}

	char *line = NULL;
	size_t line_size = 0;
	size_t read = 0;

	while(((read = getline(&line, &line_size, fp)) != -1)) {
		if(line[0] == '#') {
			continue;
		}
		char *newline = strrchr(line, '\n');
		if(newline) {
			*newline = '\0';
		}

		if(argc < argv_size) {
			argv[argc++] = strdup(line);
		} else {
			fprintf(stderr, "Error: too many config\n");
			exit(1);
		}
	}

	if(line) {
		free(line);
	}
	fclose(fp);

	return argc;
}

int config_load_config_file(struct config *conf)
{
	int argc = 0;
	char *argv[MAX_ARGV_SIZE] = {0};


	argc = config_file_to_argv(conf->config_file, argv, ARRAY_SIZE(argv));

	int ret = 0;
	ret = config_load_argument(conf, argc, argv);

	//free argv
	int i = 0;
	for(i = 0; i < argc; i++) {
		if(argv[i]) {
			//free(argv[i]);
		}
	}

	if(ret != 0) {
		fprintf(stderr, "Error: config_file is not correct\n");
	}
	return ret;

}

int config_save_config_file(struct config *conf)
{
	if(conf->config_file == NULL) {
		return -1; //can't save
	}

	FILE *fp = fopen(conf->config_file, "w+");
	if(fp == NULL) {
		return 0;
	}

	fprintf(fp, "ulog\n");

	if(conf->append) {
		fprintf(fp, "--append\n");
	}

	if(conf->banner) {
		fprintf(fp, "--banner\n");
	}

	/* config file should not contain another config file info
	if(strlen(conf->config_file)) {
		fprintf(fp, "--config\n");
		fprintf(fp, "%s\n", conf->config_file);
	}
	*/

	if(conf->debug_level) {
		fprintf(fp, "--global_debug\n");
	}

	if(strlen(conf->log_file)) {
		fprintf(fp, "--logfile\n");
		fprintf(fp, "%s\n", conf->log_file);
	}

	fprintf(fp, "--rotate\n");
	fprintf(fp, "%d\n", conf->rotate);

	fprintf(fp, "--size\n");
	fprintf(fp, "%d\n", conf->log_size);

	if(conf->timestamp_type) {
		fprintf(fp, "--timestamp\n");
	}

	if(conf->compress_type) {
		fprintf(fp, "--compress\n");
		fprintf(fp, "%s\n", compress_name[conf->compress_type]);
	}

	fclose(fp);
	return 0;
}

#include <sys/sysinfo.h>

int main(int argc, char *argv[])
{
	struct config default_config = {
		.append = 0,
		.banner = 0,
		.config_file = {0},
		.debug_level = 0,
		.log_file = {0},
		.mode = MODE_LOG_LINE,
		.rotate = DEFAULT_ROTATE,
		//.read_mode = READ_MODE_LINE,
		.log_size = DEFAULT_LOG_SIZE,
		.timestamp_type = TIMESTAMP_NONE,
		.compress_type = COMPRESS_NONE,
	};

	if(argc < 3) {
		ulog_usage(argv[0]);
		exit(1);
	}

	if(config_load_argument(&default_config, argc, argv) != 0) {
		ulog_usage(argv[0]);
		exit(1);
	}

	if(strlen(default_config.config_file)) {
		config_load_config_file(&default_config);
		config_save_config_file(&default_config);
	}

	if(strlen(default_config.log_file) == 0) {
		fprintf(stderr, "Error: no log file\n");
		exit(1);
	}

	if(strcmp(basename(argv[0]), "ulog") == 0) {
		return ulog(&default_config);
	} else if(strcmp(basename(argv[0]), "ulogread") == 0) {
		default_config.mode = MODE_READ_LOG;
		return ulog(&default_config);
	}
	return -1;
}
