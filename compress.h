#ifndef COMPRESS_H
#define COMPRESS_H

int compress_file_gzip(char *src_file, char *dst_file);
int decompress_file_zcat(char *src_file);

#endif //COMPRESS_H
