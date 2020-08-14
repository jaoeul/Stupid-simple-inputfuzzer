fuzz:
	gcc -g -Werror -Wall -o fuzz input_fuzzer.c

clean:
	rm fuzz
