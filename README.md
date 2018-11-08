# Sub Commands

A small single-file based library for common things to do in C:

  - implement a “modern” git-like command line
  - execute commands and get stdout and stderr from them
  - process file paths

## License terms

The ISC license is an Open Source license used by the OpenBSD project,
allows users to do about whatever they want to with this code.

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

## runprogram.h

A single-file C library to implement running subprograms and capturing their
output, in a safe way, with multiple pipes: both standard output and
standard error are captured. It should be trivial, and it still requires
more effort than most would like to put in.

## foo.c

A small example program that shows the API from the previous three libs.


## pqexpbuffer

This PostgreSQL facility provides a nice wrapper around dynamic allocation
of string buffers and is vendored-in here. The PostgreSQL License and the
ISC License are compatible, making that possible.
