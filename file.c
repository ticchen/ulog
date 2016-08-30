#include <sys/stat.h>

long long file_size(char *filename)
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
