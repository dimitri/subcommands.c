# Sub Commands

A small single-file based library for common things to do in C:

  - implement a “modern” git-like command line
  - execute commands and get stdout and stderr from them
  - process file paths

## License terms

The ISC license is an Open Source license used by the OpenBSD project,
allows users to do about whatever they want to with this code.

## SDS: Simple Dynamic Strings

This library vendors-in the SDS library for dynamic string handling in C.
The SDS lib is available with a BSD-2-Clause licence.

  https://github.com/antirez/sds

## commandline.h

A single-file C library to implement command line parsing with commands and
subcommands, because I failed to find an existing one that I could use
without too much dependencies.

Inspiration and thanks from https://libcork.io, which does the job quite
fine. If you can depend on an external lib at build-time and run-time,
consider using libcork.

License: ISC, https://en.wikipedia.org/wiki/ISC_license

## filepaths.h

Expose an API to process Unix file paths.

## exec.c

Use posix API to run commands: fork, exec, pipes. Allows to get stdout and
stderr of subprocesses. Only synchronous calls implemented in this small
single-file lib.

## foo.c

A small example program that shows the API from the previous three libs.
