#pragma warning(disable:4996)
#include <stdio.h>
#include <stdlib.h>
#include <sys\stat.h>

int fsize(const char *filename)
{
	struct stat st;

	if (stat(filename, &st) == 0)
		return st.st_size;

	return -1;
}

// adds null byte, but also optionally returns size
char* readfile(char* fname, int* _size)
{
	int size;
	char* buf;

	FILE* f = fopen(fname, "rb");
	if (!f) return 0;
	size = fsize(fname);
	buf = (char*)malloc(size + 1);
	fread(buf, 1, size, f);
	buf[size] = 0;
	fclose(f);
	if(_size) *_size = size;
	return buf;
}

int file_exists(char* filename)
{
    FILE* f = fopen(filename, "r");
    if(!f) return 0;
	fclose(f);
	return 1;
}
