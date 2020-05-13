#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "args.hpp"

using namespace BotBasePlus;

int Args::parseArgs(char *origargstr, int (*callback)(int, char **))
{
	char *argstr = (char *)malloc(strlen(origargstr) + 1);
	memcpy(argstr, origargstr, strlen(origargstr) + 1);
	int argc = 0;

	char *tmpstr = (char *)malloc(strlen(argstr) + 1);
	memcpy(tmpstr, argstr, strlen(argstr) + 1);
	for (char *p = strtok(tmpstr, " \r\n"); p != NULL; p = strtok(NULL, " \r\n"))
	{
		argc++;
	}
	free(tmpstr);

	if (argc == 0)
	{
		return (*callback)(argc, NULL);
	}

	char **argv = (char **)malloc(sizeof(char *) * argc);
	int i = 0;
	for (char *p = strtok(argstr, " \r\n"); p != NULL; p = strtok(NULL, " \r\n"))
		argv[i++] = p;

	int ret = (*callback)(argc, argv);
	free(argv);
	free(argstr);
	return ret;
}
