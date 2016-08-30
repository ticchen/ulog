#ifndef COMMON_H
#define COMMON_H

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))
#define array_for_each_entry_size(entry, array, array_size) \
	for (entry = array;entry<array+array_size;entry++)
#define array_for_each_entry(entry, array) array_for_each_entry_size(entry, array, ARRAY_SIZE(array))

#define ddd(fmt, ...) do \
{ \
	fprintf(stderr, "%s():%d:"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
} while(0)

int do_system(char *format, ...);
const char *format_string(const char *str, size_t str_size, const char *format, ...);

#endif //COMMON_H
