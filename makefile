system:
	gcc -Wall -g -DSYSTEM_MALLOC memtest.c -o test_sys

mem:
	gcc -Wall -g memtest.c mem.c -o test_mem

clean:
	rm -f test_sys test_mem
