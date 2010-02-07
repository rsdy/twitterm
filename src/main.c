#include "main.h"
#include "ui.h"
#include <stdio.h>

/** @file */

char *mystrdup(char *str)
{
	int len = strlen(str);
	char *ret = malloc(len * sizeof(*ret) + 1);
	memcpy(ret, str, len);
	ret[len] = 0;
	return ret;
}

/** The main() function doesn't do much, just calls the ui */
int main(int argc, char **argv)
{
	if (argc < 2)
		init_ui(NULL);
	else
		init_ui(argv[1]);

	return 0;
}
