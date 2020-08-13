fuzz:
	gcc -g -Werror -Wall -o fuzz dynamicarray.c input_fuzzer.c

clean:
	rm fuzz
