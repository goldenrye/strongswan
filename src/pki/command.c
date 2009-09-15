/*
 * Copyright (C) 2009 Martin Willi
 * Hochschule fuer Technik Rapperswil
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include "command.h"


#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Registered commands.
 */
command_t cmds[MAX_COMMANDS];

/**
 * active command.
 */
static int active = 0;

/**
 * number of registered commands
 */
static int registered = 0;

/**
 * help command index
 */
static int help_idx;

/**
 * Global options used by all subcommands
 */
struct option command_opts[MAX_COMMANDS > MAX_OPTIONS ?: MAX_OPTIONS];

/**
 * Global optstring used by all subcommands
 */
char command_optstring[(MAX_COMMANDS > MAX_OPTIONS ?: MAX_OPTIONS) * 3];

/**
 * Build command_opts/command_optstr for the active command
 */
static void build_opts()
{
	int i, pos = 0;

	memset(command_opts, 0, sizeof(command_opts));
	memset(command_optstring, 0, sizeof(command_optstring));
	if (active == help_idx)
	{
		for (i = 0; cmds[i].cmd; i++)
		{
			command_opts[i].name = cmds[i].cmd;
			command_opts[i].val = cmds[i].op;
			command_optstring[i] = cmds[i].op;
		}
	}
	else
	{
		for (i = 0; cmds[active].options[i].name; i++)
		{
			command_opts[i].name = cmds[active].options[i].name;
			command_opts[i].has_arg = cmds[active].options[i].arg;
			command_opts[i].val = cmds[active].options[i].op;
			command_optstring[pos++] = cmds[active].options[i].op;
			switch (cmds[active].options[i].arg)
			{
				case optional_argument:
					command_optstring[pos++] = ':';
					/* FALL */
				case required_argument:
					command_optstring[pos++] = ':';
					/* FALL */
				case no_argument:
				default:
					break;
			}
		}
	}
}

/**
 * Register a command
 */
void command_register(command_t command)
{
	cmds[registered++] = command;
}

/**
 * Print usage text, with an optional error
 */
int command_usage(char *error)
{
	FILE *out = stdout;
	char buf[64];
	int i;

	if (error)
	{
		out = stderr;
		fprintf(out, "Error: %s\n", error);
	}
	fprintf(out, "strongSwan %s PKI tool\n", VERSION);
	fprintf(out, "usage:\n");
	if (active == help_idx)
	{
		for (i = 0; cmds[i].cmd; i++)
		{
			snprintf(buf, sizeof(buf), "--%s (-%c)", cmds[i].cmd, cmds[i].op);
			fprintf(out, "  pki %-14s %s\n", buf, cmds[i].description);
		}
	}
	else
	{
		for (i = 0; cmds[active].line[i]; i++)
		{
			if (i == 0)
			{
				fprintf(out, "  pki --%s %s\n",
						cmds[active].cmd, cmds[active].line[i]);
			}
			else
			{
				fprintf(out, "              %s\n", cmds[active].line[i]);
			}
		}
		for (i = 0; cmds[active].options[i].name; i++)
		{
			snprintf(buf, sizeof(buf), "--%s (-%c)",
					 cmds[active].options[i].name, cmds[active].options[i].op);
			fprintf(out, "        %-15s %s\n",
					buf, cmds[active].options[i].desc);
		}
	}
	return error != NULL;
}


/**
 * Show usage information
 */
static int help(int argc, char *argv[])
{
	return command_usage(NULL);
}

/**
 * Dispatch commands.
 */
int command_dispatch(int argc, char *argv[])
{
	int op, i;

	active = help_idx = registered;
	command_register((command_t){help, 'h', "help", "show usage information"});

	build_opts();
	op = getopt_long(argc, argv, command_optstring, command_opts, NULL);
	for (i = 0; cmds[i].cmd; i++)
	{
		if (cmds[i].op == op)
		{
			active = i;
			build_opts();
			return cmds[i].call(argc, argv);
		}
	}
	return command_usage("invalid operation");
}

