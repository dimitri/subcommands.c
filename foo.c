/*
 * subcommands.c, copyright Dimitri Fontaine <dim@tapoueh.org>
 * License: ISC
 *
 * Example program.
 */

#include <stdlib.h>
#include <stdio.h>

#include "commandline.h"

static void main_env(int argc, char **argv);
static void main_env_get(int argc, char **argv);
static void main_env_put(int argc, char **argv);

static void main_ls(int argc, char **argv);
static int ls_getopt(int argc, char **argv);

static void main_cat(int argc, char **argv);
static int cat_getopt(int argc, char **argv);

static void main_exec(int argc, char **argv);

cmd_t env_cmd_get = make_command("get",
                                 "get env variable value",
								 "<variable name>",
								 NULL,
                                 NULL, &main_env_get);

cmd_t env_cmd_put = make_command("put",
                                 "put env variable value", NULL, NULL,
                                 NULL, &main_env_put);

cmd_t *env_cmd_set[] = {
  &env_cmd_get,
  &env_cmd_put,
  NULL
};

cmd_t env_cmd = make_command_set("env", "access environment", NULL, NULL,
                                 NULL, env_cmd_set);

cmd_t ls_cmd = make_command("ls",
                            "list file or directory", "[-alr]", NULL,
                            &ls_getopt, &main_ls);

/* cmd_t cat_cmd = make_command("cat" */
/*                              "cat file", "[-n]", NULL, */
/*                              &cat_getopt, &main_cat); */

cmd_t exec_cmd = make_command("exec",
                              "execute command", NULL, NULL,
                              NULL, &main_exec);

cmd_t *main_cmd_set[] = {
  &env_cmd,
  &ls_cmd,
  /* &cat_cmd, */
  &exec_cmd,
  NULL
};

cmd_t main_cmd = make_command_set("foo",
                                  "test program for subcommands.c",
                                  NULL, NULL,
                                  NULL, main_cmd_set);

int
main(int argc, char **argv)
{
  commandline_run(&main_cmd, argc, argv);

  return 0;
}


static void
main_env(int argc, char **argv)
{
  fprintf(stdout, "entering %s\n", __func__);
  return;
}


static void
main_env_get(int argc, char **argv)
{
	char *val;

	if (argc == 1)
    {
		char *val = getenv(argv[0]);

		fprintf(stdout, "%s\n", val);
		fflush(stdout);
    }
	else
	{
		commandline_print_usage(&env_cmd_get, stderr);
		exit(1);
	}
	return;
}


static void
main_env_put(int argc, char **argv)
{
  fprintf(stdout, "entering %s", __func__);
  return;
}


static int
ls_getopt(int argc, char **argv)
{
  fprintf(stdout, "entering %s", __func__);
  return 1;
}


static void
main_ls(int argc, char **argv)
{
  fprintf(stdout, "entering %s", __func__);
  return;
}


static void
main_cat(int argc, char **argv)
{
  fprintf(stdout, "entering %s", __func__);
  return;
}


static int
cat_getopt(int argc, char **argv)
{
  fprintf(stdout, "entering %s", __func__);
  return 1;
}


static void
main_exec(int argc, char **argv)
{
  fprintf(stdout, "entering %s", __func__);
  return;
}
