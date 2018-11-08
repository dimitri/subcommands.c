/*
 * subcommands.c, copyright Citus Data, Inc.
 * License: ISC
 *
 * Example program.
 */

#include <getopt.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define COMMAND_LINE_IMPLEMENTATION
#include "commandline.h"

#define FILEPATHS_IMPLEMENTATION
#include "filepaths.h"

#define RUN_PROGRAM_IMPLEMENTATION
#include "runprogram.h"

static bool ls_opt_all = false;
static bool ls_opt_long = false;
static bool ls_opt_recursive = false;

static void main_env_get(int argc, char **argv);
static void main_env_set(int argc, char **argv);

static void main_path_ls(int argc, char **argv);
static void main_path_ext(int argc, char **argv);
static void main_path_join(int argc, char **argv);
static void main_path_joindir(int argc, char **argv);
static void main_path_merge(int argc, char **argv);
static void main_path_rel(int argc, char **argv);
static void main_path_mkdirs(int argc, char **argv);
static void main_path_rmdir(int argc, char **argv);
static void main_path_find(int argc, char **argv);
static void main_path_abs(int argc, char **argv);

static void main_ls(int argc, char **argv);
static int ls_getopt(int argc, char **argv);

static void main_which(int argc, char **argv);
static void main_echo12(int argc, char **argv);

CommandLine env_cmd_get = make_command("get",
									   "get env variable value",
									   "<variable name>",
									   NULL,
									   NULL, &main_env_get);

CommandLine env_cmd_set = make_command("set",
									   "set env variable value", NULL, NULL,
									   NULL, &main_env_set);

CommandLine *env_cmds[] = {
	&env_cmd_get,
	&env_cmd_set,
	NULL
};

CommandLine env_cmd = make_command_set("env", "access environment", NULL, NULL,
									   NULL, env_cmds);

CommandLine path_cmd_ls = make_command("ls",
									   "list a filepath",
									   "<filename> [ ... ]",
									   NULL,
									   NULL, &main_path_ls);

CommandLine path_cmd_ext = make_command("ext",
										"change extension of a filepath",
										"<filename> <extension>",
										NULL,
										NULL, &main_path_ext);

CommandLine path_cmd_join = make_command("join",
										 "join file paths",
										 "<filename a> <filename b>",
										 NULL,
										 NULL, &main_path_join);

CommandLine path_cmd_joindir = make_command("joindir",
											"join file paths to make a subdirectory",
											"<dir a> <subdir>",
											NULL,
											NULL, &main_path_joindir);

CommandLine path_cmd_merge = make_command("merge",
										  "merge file paths",
										  "<specs> <defaults>",
										  NULL,
										  NULL, &main_path_merge);

CommandLine path_cmd_rel = make_command("rel",
										"returns relative path from root to target",
										"<target> <root>",
										NULL,
										NULL, &main_path_rel);

CommandLine path_cmd_mkdirs = make_command("mkdirs",
										   "ensure target directory exists",
										   "<target>",
										   NULL,
										   NULL, &main_path_mkdirs);

CommandLine path_cmd_rmdir = make_command("rmdir",
										  "delete target directory and its contents",
										  "<target>",
										  NULL,
										  NULL, &main_path_rmdir);

CommandLine path_cmd_find = make_command("find",
										 "find all files in PATH",
										 "<filename>",
										 NULL,
										 NULL, &main_path_find);

CommandLine path_cmd_abs = make_command("abs",
										"get absolute filename",
										"<filename>",
										NULL,
										NULL, &main_path_abs);

CommandLine *path_cmds[] = {
	&path_cmd_ls,
	&path_cmd_ext,
	&path_cmd_join,
	&path_cmd_joindir,
	&path_cmd_merge,
	&path_cmd_rel,
	&path_cmd_mkdirs,
	&path_cmd_rmdir,
	&path_cmd_find,
	&path_cmd_abs,
	NULL
};

CommandLine path_cmd = make_command_set("path", "compose path names", NULL, NULL,
										NULL, path_cmds);

CommandLine ls_cmd = make_command("ls",
								  "list file or directory", "[-alr]", NULL,
								  &ls_getopt, &main_ls);

/* CommandLine cat_cmd = make_command("cat" */
/*                              "cat file", "[-n]", NULL, */
/*                              &cat_getopt, &main_cat); */

CommandLine which_cmd = make_command("which",
									 "run /usr/bin/which", "<program>", NULL,
									 NULL, &main_which);

CommandLine echo_cmd = make_command("echo",
									"run /usr/bin/echo", "<nb>", NULL,
									NULL, &main_echo12);

CommandLine *main_cmds[] = {
	&env_cmd,
	&path_cmd,
	&ls_cmd,
	/* &cat_cmd, */
	&which_cmd,
	&echo_cmd,
	NULL
};

CommandLine main_cmd = make_command_set("foo",
										"test program for subcommands.c",
										NULL, NULL,
										NULL, main_cmds);

int
main(int argc, char **argv)
{
	commandline_run(&main_cmd, argc, argv);

	return 0;
}

/*
 * Environment utils.
 *
 * Only interest to put that here is that the code is easy to write and should
 * work about everywhere.
 *
 *  ./foo env get name
 *  ./foo env set name value
 *
 * Of course the "set" command is pretty useless, as the environment variable
 * is set of the duration of the ./foo execution, and that's the only thing
 * this command does. That's the limit of this example program.
 */
static void
main_env_get(int argc, char **argv)
{
	if (argc == 1)
    {
		char *val = getenv(argv[0]);

		if (val != NULL)
		{
			fprintf(stdout, "%s\n", val);
			fflush(stdout);
		}
		else
		{
			fprintf(stderr, "Environment variable \"%s\" is not set\n", argv[0]);
			fflush(stdout);
			exit(1);
		}
    }
	else
	{
		commandline_help(stderr);
		exit(1);
	}
	return;
}


static void
main_env_set(int argc, char **argv)
{
	if (argc == 2)
	{
		char *name = argv[0];
		char *value = argv[1];

		if (setenv(name, value, 1) == 0)
		{
			fprintf(stdout, "%s\n", value);
			fflush(stdout);
		}
		else
		{
			fprintf(stderr,
					"Failed to set environment variable \"%s\": %s",
					name,
					strerror(errno));
			fflush(stderr);
			exit(1);
		}
	}
	else
	{
		commandline_help(stderr);
		exit(1);
	}
	return;
}

/*
 * foo path
 *
 * Test the filepaths library, exposing its feature set.
 */
static void
main_path_ls(int argc, char **argv)
{
	if (argc == 0)
	{
		commandline_help(stderr);
		exit(1);
	}

	for (int i = 0; i < argc; i++)
	{
		Path *path = filepath_new(argv[i]);

		printf("filename: %s\n", path->filename);
		printf("realpath: %s\n", path->realpath);
		printf("    name: %s\n", path->name);
		printf("    .ext: %s\n", path->extension);
		printf("    stat: %s\n", path->exists ? "exists" : "does not exists");
		printf("  is dir: %s\n", filepath_is_dir(path) ? "yes" : "no");
		printf("\n");
		filepath_fprintf(stdout, path);
		printf("\n\n");

		filepath_free(path);
	}
	return;
}


static void
main_path_ext(int argc, char **argv)
{
	if (argc == 2)
	{
		Path *path = filepath_new(argv[0]);
		char *new_extension = argv[1];
		Path *newp;

		if (strlen(new_extension) > 1 && new_extension[0] == '.')
		{
			new_extension++;
		}
		newp = filepath_with_extension(path, new_extension);

		printf("filename: %s\n", newp->filename);
		printf("realpath: %s\n", newp->realpath);
		printf("    name: %s\n", newp->name);
		printf("    .ext: %s\n", newp->extension);
		printf("\n");
		filepath_fprintf(stdout, newp);
		printf("\n\n");

		filepath_free(path);
		filepath_free(newp);

		return;
	}
	else
	{
		commandline_help(stderr);
		exit(1);
	}
	return;
}


static void
main_path_join(int argc, char **argv)
{
	if (argc == 2)
	{
		Path *path = filepath_new(argv[0]);
		Path *join = filepath_join(path, argv[1]);

		printf("  file a: %s\n", path->filename);
		printf("  file b: %s\n", argv[1]);
		printf("filename: %s\n", join->filename);
		printf("realpath: %s\n", join->realpath);
		printf("    name: %s\n", join->name);
		printf("    .ext: %s\n", join->extension);
		printf("\n");
		filepath_fprintf(stdout, join);
		printf("\n\n");

		filepath_free(path);
		filepath_free(join);

		return;
	}
	else
	{
		commandline_help(stderr);
		exit(1);
	}
	return;
}


static void
main_path_joindir(int argc, char **argv)
{
	if (argc == 2)
	{
		Path *path = filepath_new(argv[0]);
		Path *join = filepath_join_subdir(path, argv[1]);

		printf("  file a: %s\n", filepath_get_filename(path));
		printf("  file b: %s\n", argv[1]);
		printf("filename: %s\n", join->filename);
		printf("realpath: %s\n", join->realpath);
		printf("    name: %s\n", join->name);
		printf("    .ext: %s\n", join->extension);
		printf("    stat: %s\n", join->exists ? "exists" : "does not exists");
		printf("  is dir: %s\n", filepath_is_dir(join) ? "yes" : "no");
		printf("\n");
		filepath_fprintf(stdout, join);
		printf("\n\n");

		filepath_free(path);
		filepath_free(join);

		return;
	}
	else
	{
		commandline_help(stderr);
		exit(1);
	}
	return;
}


static void
main_path_merge(int argc, char **argv)
{
	if (argc == 2)
	{
		Path *specs = filepath_new(argv[0]);
		Path *defaults = filepath_new(argv[1]);
		Path *merge = filepath_merge(specs, defaults);

		printf("filename: %s\n", merge->filename);
		printf("realpath: %s\n", merge->realpath);
		printf("    name: %s\n", merge->name);
		printf("    .ext: %s\n", merge->extension);
		printf("\n");
		filepath_fprintf(stdout, merge);
		printf("\n\n");

		filepath_free(specs);
		filepath_free(defaults);
		filepath_free(merge);

		return;
	}
	else
	{
		commandline_help(stderr);
		exit(1);
	}
	return;
}

static void
main_path_rel(int argc, char **argv)
{
	if (argc == 2)
	{
		Path *target = filepath_new(argv[0]);
		Path *root = filepath_new(argv[1]);
		const char *relative_filename =
			filepath_relative_filename(target, root);

		printf("%s\n", relative_filename);

		filepath_free(target);
		filepath_free(root);

		return;
	}
	else
	{
		commandline_help(stderr);
		exit(1);
	}
	return;
}


static void
main_path_mkdirs(int argc, char **argv)
{
	if (argc == 1)
	{
		Path *target = filepath_newdir(argv[0]);
		bool created = filepath_ensure_directories_exist(target, 0755);

		if (!created)
		{
			fprintf(stderr,
					"Failed to create \"%s\": %s\n",
					argv[0],
					strerror(errno));
			exit(1);
		}

		printf("get name: %s\n", filepath_get_filename(target));
		printf("filename: %s\n", target->filename);
		printf("realpath: %s\n", target->realpath);
		printf("    name: %s\n", target->name);
		printf("    .ext: %s\n", target->extension);
		printf("    stat: %s\n", target->exists ? "exists" : "does not exists");
		printf("  is dir: %s\n", filepath_is_dir(target) ? "yes" : "no");
		printf("\n");
		filepath_fprintf(stdout, target);
		printf("\n\n");

		filepath_free(target);
	}
	else
	{
		commandline_help(stderr);
		exit(1);
	}
	return;
}


static void
main_path_rmdir(int argc, char **argv)
{
	if (argc == 1)
	{
		Path *target = filepath_new(argv[0]);
		bool deleted = filepath_remove_directory(target);

		if (!deleted)
		{
			fprintf(stderr,
					"Failed to delete \"%s\": %s\n",
					argv[0],
					strerror(errno));
			exit(1);
		}

		printf("deleted directory \"%s\"\n\n", target->realpath);
		fflush(stdout);
		filepath_free(target);
	}
	else
	{
		commandline_help(stderr);
		exit(1);
	}
	return;
}


static void
main_path_find(int argc, char **argv)
{
	if (argc == 1)
	{
		PathList *path = filepath_list_new(getenv("PATH"));
		PathList *matches = filepath_list_find(path, argv[0]);

		for (int i = 0; i < matches->size; i++)
		{
			char *filename = filepath_get_filename(matches->list[i]);

			fprintf(stdout, "%s\n", filename);
		}
		fflush(stdout);

		filepath_list_free(path);
		filepath_list_free(matches);
	}
	else
	{
		commandline_help(stderr);
		exit(1);
	}
	return;
}


static void
main_path_abs(int argc, char **argv)
{
	if (argc == 1)
	{
		Path *p = filepath_new(argv[0]);
		const char *pathname;

		if (filepath_file_exists(p))
		{
			pathname = filepath_get_filename(p);
		}
		else if (filepath_is_absolute(p))
		{
			pathname = p->filename;
		}
		else
		{
			Path *cwd = filepath_cwd();
			Path *new = filepath_merge(p, cwd);

			pathname = filepath_get_filename(new);

			filepath_free(cwd);
			filepath_free(new);
		}

		printf("filename: %s\n", p->filename);
		printf("realpath: %s\n", p->realpath);
		printf("    name: %s\n", p->name);
		printf("    .ext: %s\n", p->extension);
		printf("    stat: %s\n", p->exists ? "exists" : "does not exists");
		printf("  is dir: %s\n", filepath_is_dir(p) ? "yes" : "no");
		printf("\n");
		printf("%s\n\n", pathname);

		filepath_free(p);
	}
	else
	{
		commandline_help(stderr);
		exit(1);
	}
	return;
}


/*
 * foo ls
 *
 * Command to list files. This command is coded so as to show case parts of the
 * filepaths.h API.
 */
static int
ls_getopt(int argc, char **argv)
{
	static struct option long_options[] = {
		{"all", no_argument, NULL, 'a'},
		{"long", no_argument, NULL, 'l'},
		{"recursive", no_argument, NULL, 'r'},
		{NULL, 0, NULL, 0}
	};

	int c, option_index, errors = 0;

	optind = 0;

	while ((c = getopt_long(argc, argv, "alr",
							long_options, &option_index)) != -1)
	{
		switch (c)
		{
			case 'a':
				ls_opt_all = true;
				break;

			case 'l':
				ls_opt_long = true;
				break;

			case 'r':
				ls_opt_recursive = true;
				break;

			default:
			{
				fprintf(stderr, "Unknown option \"%c\"\n", c);
				errors++;
				break;
			}
		}
	}

	if (errors > 0)
	{
		commandline_help(stderr);
		exit(1);
	}
	return optind;
}


static void
main_ls(int argc, char **argv)
{
	if (argc == 0)
	{
		Path *cwd = filepath_cwd();

		filepath_fprintf(stdout, cwd);
		filepath_free(cwd);
	}
	else
	{
		for (int i = 0; i < argc; i++)
		{
			char *filename = argv[i];
			Path *path;

			fprintf(stdout, "%s\n", argv[i]);

			path = filepath_new(filename);
			filepath_fprintf(stdout, path);
			filepath_free(path);
		}
	}
	fprintf(stdout, "\n");
	fflush(stdout);
	return;
}


static void
main_which(int argc, char **argv)
{
	if (argc == 1)
	{
		Program prog = run_program("/usr/bin/which", argv[0], NULL);
		int rc = prog.returnCode;

		if (prog.error != 0)
		{
			fprintf(stderr, "Failed to run program \"%s\": %s\n",
					prog.program, strerror(prog.error));
			fflush(stderr);
			exit(1);
		}
		fprintf(stdout, "main_which: %p %ld\n", prog.stdout, strlen(prog.stdout));

		if (prog.stdout != NULL)
		{
			fprintf(stdout, "%s\n", prog.stdout);
		}

		if (prog.stderr != NULL)
		{
			fprintf(stderr, "%s\n", prog.stderr);
		}

		fflush(stdout);
		fflush(stderr);

		free_program(&prog);

		exit(rc);
	}
	else
	{
		commandline_help(stderr);
		exit(1);
	}

	return;
}


static void
main_echo12(int argc, char **argv)
{
	if (argc == 1)
	{
		Program prog;
		int nb = atoi(argv[0]);

		if (nb == 0 && errno == EINVAL)
		{
			fprintf(stderr,
					"Failed to parse number \"%s\": %s\n",
					argv[0], strerror(errno));
			exit(1);
		}

		switch (nb)
		{
			case 1:
				prog = run_program("/bin/echo", "1", NULL);
				break;

			case 2:
				prog = run_program("/bin/echo", "1", "2", NULL);
				break;

			case 12:
				prog = run_program("/bin/echo",
								   "1", "2", "3", "4", "5", "6",
								   "7", "8", "9", "a", "b", "c",
								   NULL);
				break;

			case 13:
				prog = run_program("/bin/echo",
								   "1", "2", "3", "4", "5", "6",
								   "7", "8", "9", "a", "b", "c", "d",
								   NULL);
				break;

			case 15:
				prog = run_program("/bin/echo",
								   "1", "2", "3", "4", "5", "6",
								   "7", "8", "9", "a", "b", "c", "d", "e", "f",
								   NULL);
				break;

			default:
				fprintf(stderr, "Number of arguments not supported: %d\n", nb);
				exit(1);
				break;
		}

		if (prog.error != 0)
		{
			fprintf(stderr, "Failed to run program \"%s\": %s\n",
					prog.program, strerror(prog.error));
			fflush(stderr);
			exit(1);
		}

		if (prog.stdout != NULL)
		{
			fprintf(stdout, "%s\n", prog.stdout);
		}

		if (prog.stderr != NULL)
		{
			fprintf(stderr, "%s\n", prog.stderr);
		}

		fflush(stdout);
		fflush(stderr);

		exit(prog.returnCode);
	}
	else
	{
		commandline_help(stderr);
		exit(1);
	}

	return;
}
