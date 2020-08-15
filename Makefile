fuzz:
	gcc -g -pthread -Werror -Wall -o fuzz input_fuzzer.c

clean:
	rm fuzz
