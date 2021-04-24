#define _CRT_NONSTDC_NO_WARNINGS // strdup -> _strdup
#define _CRT_SECURE_NO_WARNINGS  // strcat -> strcat_s
#include <stdlib.h>
#include <string.h>
void error(char* str);

void split(char* str, char sep, char*** result, int* count)
{
	int n = 1;
	char* p = str;
	while (*p) { if (*p == sep) n++; p++; }

	char** ary = calloc(n, sizeof(char*));
	int i = 0;

	str = strdup(str); // we modify the string, need to duplicate
	p = str;
	char* end; // end of substring which starts at p
	while(end = strchr(p, sep)) // intended assignment
	{
		*end = 0;
		// duplicate each substring so elements can be used independently
		ary[i++] = strdup(p);
		p = end+1;
	}
	// from last sep to end of string or entire str if sep not found
	ary[i++] = strdup(p);

	if (i != n) error("internal error in split()");
	free(str);
	*result = ary;
	*count = n;
}

char* join(char** ary, int count, char* sep)
{
	if (count <= 0) return NULL;
	if (count == 1) return strdup(ary[0]);

	// else: count >= 2
	char* str = strdup(ary[0]);
	for (int i = 1; i < count; i++)
	{
		str = realloc(str, strlen(str) + strlen(sep) + strlen(ary[i]) + 1); // +1: '\0'
		strcat(str, sep);
		strcat(str, ary[i]);
	}
	return str;
}