#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <zlib.h>

#include "common.h"
#include "file.h"


int compress_file_gzip(char *src_file, char *dst_file)
{
	int ret = 0;

	struct gzFile_s *gz;
	gz = gzopen(dst_file, "w");
	if(gz == NULL) {
		fprintf(stderr, "Error: fail to create gzip file: %s\n", dst_file);
		return -1;
	}

	do {
		FILE *fp = fopen(src_file, "r");
		if(fp == NULL) {
			ret = -1;
			break;
		}
		char buff[1024] = {0};
		size_t len = 0;
		while(!feof(fp)) {
			len = fread(buff, 1, sizeof(buff), fp);
			gzwrite(gz, buff, len);
		}

		fclose(fp);
	} while(0);

	if(gz) {
		gzclose(gz);
	}

	return ret;
}


int decompress_file_zcat(char *src_file)
{
	int ret = 0;

	struct gzFile_s *gz;
	gz = gzopen(src_file, "rb");
	if(gz == NULL) {
		fprintf(stderr, "Error: fail to read gzip file: %s\n", src_file);
		return -1;
	}

	char buff[1024] = {0};
	unsigned int len = 0;
	int read = 0;

	while(!gzeof(gz)) {
		read = gzread(gz, buff, sizeof(buff));
		int errno;
		if(read > 0) {
			fwrite(buff, 1, read, stdout);
		} else {
			break;
		}
	}
	gzclose(gz);
	return 0;
}
