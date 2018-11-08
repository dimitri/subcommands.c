/*
 * commandline.h, copyright Citus Data, Inc.
 * License: ISC
 *
 */

#ifdef COMMAND_LINE_IMPLEMENTATION
#undef COMMAND_LINE_IMPLEMENTATION

#include <string.h>

typedef int (*command_getopt)(int argc, char **argv);
typedef void (*command_run)(int argc, char **argv);

typedef struct command_line cmd_t;

struct command_line
{
	const char *name;
	const char *short_desc;
	const char *usage_suffix;
	const char *help;

	command_getopt	getopt;
	command_run		run;

	struct command_line **subcommands;
	char *breadcrumb;
};

static cmd_t *current_command = NULL;

#define make_command_set(name, desc, usage, help, getopt, set)	\
	{name, desc, usage, help, getopt, NULL, set, NULL}

#define make_command(name, desc, usage, help, getopt, run)	\
	{name, desc, usage, help, getopt, run, NULL, NULL}

void commandline_run(cmd_t *command, int argc, char **argv);
void commandline_help(FILE *stream);
void commandline_print_usage(cmd_t *command, FILE *stream);
void commandline_print_subcommands(cmd_t *command, FILE *stream);
void commandline_add_breadcrumb(cmd_t *command, cmd_t *subcommand);

#define streq(a, b) (a != NULL && b != NULL && strcmp(a, b) == 0)

/*
 * Implementation of the main subcommands entry point.
 *
 * Parses the command line given the Command_t cmd context, and run commands
 * that match with the subcommand definitions.
 */
void
commandline_run(cmd_t *command, int argc, char **argv)
{
	const char *argv0 = NULL;

	if (argc > 0)
    {
		argv0 = argv[0];
    }

	/*
	 * If the user gives the --help option at this point, describe the current
	 * command.
	 */
	if (argc >= 2 && (streq(argv[1], "--help") || streq(argv[1], "-h")))
    {
		commandline_print_usage(command, stderr);
		return;
    }

	/* Otherwise let the command parse any options that occur here. */
	if (command->getopt != NULL)
    {
		int  option_count = command->getopt(argc, argv);
		argc -= option_count;
		argv += option_count;
    }
	else
    {
		argc--;
		argv++;
    }

	if (command->run != NULL)
    {
		current_command = command;
		return command->run(argc, argv);
    }
	else if (argc == 0)
	{
		/*
		 * We're at the end of the command line already, and command->run is
		 * not set, which means we expected a subcommand to be used, but none
		 * have been given by the user. Inform him.
		 */
		commandline_print_subcommands(command, stderr);
	}
	else
    {
		if (command->subcommands != NULL)
        {
			cmd_t **subc;

			for(subc = command->subcommands; *subc != NULL; subc++)
            {
				if (streq(argv[0], (*subc)->name))
                {
					commandline_add_breadcrumb(command, *subc);

					return commandline_run(*subc, argc, argv);
                }
            }

			/* if we reach this code, we didn't find a subcommand */
			{
				const char *bc =
					command->breadcrumb == NULL ? argv0 : command->breadcrumb;

				fprintf(stderr, "%s: %s: unknown command\n", bc, argv[0]);
			}

			fprintf(stderr, "\n");
			commandline_print_subcommands(command, stderr);
        }
    }
	return;
}


/*
 * Print help message for the known currently running command.
 */
void
commandline_help(FILE *stream)
{
	if (current_command != NULL)
	{
		commandline_print_usage(current_command, stream);
	}
	return;
}


/*
 * Helper function to print usage and help message for a command.
 */
void
commandline_print_usage(cmd_t *command, FILE *stream)
{
	const char *breadcrumb =
		command->breadcrumb == NULL ? command->name : command->breadcrumb;

	fprintf(stream, "%s:", breadcrumb);

	if (command->short_desc)
	{
		fprintf(stream, " %s", command->short_desc);
	}
	fprintf(stream, "\n");

	if (command->usage_suffix)
	{
		fprintf(stream,
				"usage: %s %s\n", breadcrumb, command->usage_suffix);
		fprintf(stream, "\n");
	}

	if (command->help)
	{
		fprintf(stream, "%s\n", command->help);
	}

	if (command->subcommands)
	{
		fprintf(stream, "\n");
		commandline_print_subcommands(command, stream);
	}
	fflush(stream);

	return;
}


/*
 * Print the list of subcommands accepted from a command.
 */
void
commandline_print_subcommands(cmd_t *command, FILE *stream)
{
	/* the root command doesn't have a breadcrumb at this point */
	const char *breadcrumb =
		command->breadcrumb == NULL ? command->name : command->breadcrumb;

	fprintf(stream, "Available commands:\n  %s\n", breadcrumb);

	if (command->subcommands != NULL)
	{
		cmd_t **subc;
		int maxl = 0;

		/* pretty printing: reduce maximum length of subcommand names */
		for(subc = command->subcommands; *subc != NULL; subc++)
		{
			int len = strlen((*subc)->name);

			if (maxl < len)
			{
				maxl = len;
			}
		}

		for(subc = command->subcommands; *subc != NULL; subc++)
		{
			fprintf(stream,
					"  %c %*s  %s\n",
					(*subc)->subcommands ? '+' : ' ',
					(int) -maxl,
					(*subc)->name,
					command->short_desc ? (*subc)->short_desc : "");
		}
	}
	fprintf(stream, "\n");
}


/*
 * Add command to the breadcrumb of subcommand.
 *
 * The idea is to be able to print the list of subcommands in the help
 * messages, as in the following example:
 *
 *   $ ./foo env get --help
 *   foo env get: short description
 */
void
commandline_add_breadcrumb(cmd_t *command, cmd_t *subcommand)
{
	const char *command_bc =
		command->breadcrumb ? command->breadcrumb : command->name;
	int command_bc_len = strlen(command_bc);
	int subcommand_len = strlen(subcommand->name);
	int bc_len = command_bc_len + subcommand_len + 2;

	subcommand->breadcrumb = (char *) malloc(bc_len * sizeof(char));
	sprintf(subcommand->breadcrumb, "%s %s", command_bc, subcommand->name);

	return;
}

#endif  /* COMMAND_LINE_IMPLEMENTATION */
