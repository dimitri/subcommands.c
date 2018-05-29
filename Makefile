all: foo ;

foo: $(wildcard *.h) foo.c
	gcc -O0 -g foo.c -o $@

clean:
	rm -f foo

tree: foo
	./foo path mkdirs /tmp/sub/dir/test
	touch /tmp/sub/{a,b,c.h}
	touch /tmp/sub/dir/{toto,titi,tata}
	./foo path mkdirs /tmp/sub/sub
	touch /tmp/sub/sub/{foo,bar,baz}
	touch /tmp/sub/dir/test/coucou.txt
	tree /tmp/sub

.PHONY: all clean tree
