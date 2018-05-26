all: foo ;

foo: commandline.h foo.c
	gcc foo.c -o $@

clean:
	rm -f foo

.PHONY: all clean
