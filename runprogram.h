/*
 * runprogram.h, copyright Dimitri Fontaine <dim@tapoueh.org>
 * License: ISC
 *
 */

#ifdef RUN_PROGRAM_IMPLEMENTATION
#undef RUN_PROGRAM_IMPLEMENTATION

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "sds.h"

#define BUFSIZE			1024
#define ARGS_INCREMENT	12

typedef struct
{
	char *program;
	char **args;

	int error;
	int rc;

	sds out;
	sds err;
} Program;

struct arglist
{
	char *arg;
	struct arglist *next;
};

Program run_program(const char *program, ...);
Program run_program_internal(Program prog);

static ssize_t read_into_sds(int filedes, sds *buffer);


/*
 * Run a program using fork() and exec(), get the stdout and stderr output from
 * the run and then return a Program struct instance with the result of running
 * the program.
 */
Program
run_program(const char *program, ...)
{
	int nb_args = 0;
	va_list args;
    const char *param;
	Program prog;

	prog.program = strdup(program);
	prog.rc = -1;
	prog.error = 0;

	prog.args = (char **) malloc(ARGS_INCREMENT * sizeof(char *));
	prog.args[nb_args++] = prog.program;

    va_start(args, program);
    while ((param = va_arg(args, const char *)) != NULL)
	{
		if (nb_args % ARGS_INCREMENT == 0)
		{
			char **newargs = (char **) malloc((ARGS_INCREMENT
											   * (nb_args / ARGS_INCREMENT +1))
											  * sizeof(char *));
			for (int i = 0; i < nb_args; i++)
			{
				newargs[i] = prog.args[i];
			}
			free(prog.args);

			prog.args = newargs;
		}
		prog.args[nb_args++] = strdup(param);
    }
	va_end(args);
	prog.args[nb_args] = NULL;

	return run_program_internal(prog);
}


/*
 * Run given program with its args, by doing the fork()/exec() dance, and also
 * capture the subprocess output by installing pipes. We accimulate the output
 * into an SDS data structure (Simple Dynamic Strings library).
 */
Program
run_program_internal(Program prog)
{
	pid_t pid;
    int outpipe[2] = {0,0};
    int errpipe[2] = {0,0};

	prog.out = sdsempty();
	prog.err = sdsempty();

	/* Flush stdio channels just before fork, to avoid double-output problems */
	fflush(stdout);
	fflush(stderr);

	/* create the pipe now */
    if (pipe(outpipe) < 0)
	{
		prog.error = errno;
        return prog;
    }

    if (pipe(errpipe) < 0)
	{
		prog.error = errno;
        return prog;
    }

	pid = fork();

	switch (pid)
	{
		case -1:
		{
			/* fork failed */
			prog.error = errno;
			return prog;
		}

		case 0:
		{
			/* fork succeeded, in child */
			int stdin = open("/dev/null", O_RDONLY);

			dup2(stdin, STDIN_FILENO);
			dup2(outpipe[1], STDOUT_FILENO);
			dup2(errpipe[1], STDERR_FILENO);

			close(stdin);
			close(outpipe[0]);
			close(outpipe[1]);
			close(errpipe[0]);
			close(errpipe[1]);

			if (execv(prog.program, prog.args) == -1)
			{
				prog.error = errno;
			}
			return prog;
		}

		default:
		{
			/* fork succeeded, in parent */
			int status;
			ssize_t bytes_out = BUFSIZE, bytes_err = BUFSIZE;

			/* We read from the other side of the pipe, close that part.  */
			close(outpipe[1]);
			close(errpipe[1]);

			/*
			 * First, wait until the child process is done.
			 */
			do
			{
				if (waitpid(pid, &status, WUNTRACED) == -1)
				{
					prog.error = errno;
					return prog;
				}
			}
			while (!WIFEXITED(status));

			prog.rc = WEXITSTATUS(status);

			/*
			 * Ok. the child process is done, let's read the pipes content.
			 */
			while (bytes_out == BUFSIZE || bytes_err == BUFSIZE)
			{
				/* only continue until we're done, right? */
				if (bytes_out == BUFSIZE)
				{
					bytes_out = read_into_sds(outpipe[0], &prog.out);
				}

				if (bytes_err == BUFSIZE)
				{
					bytes_err = read_into_sds(errpipe[0], &prog.err);
				}
			}
			close(outpipe[0]);
			close(errpipe[0]);

			return prog;
		}
	}
	return prog;
}


/*
 * Read from a file descriptor and directly appends to our buffer string.
 */
static ssize_t
read_into_sds(int filedes, sds *buffer)
{
	size_t len = sdslen(*buffer);
	ssize_t bytes;

	*buffer = sdsMakeRoomFor(*buffer, BUFSIZE);
	bytes = read(filedes, *buffer + len, BUFSIZE);

	if (bytes > 0)
	{
		sdsIncrLen(*buffer, bytes);
	}
	return bytes;
}


#endif  /* RUN_PROGRAM_IMPLEMENTATION */
