main: main.c
	gcc -o main main.c -Wall -Wextra -lm -lpthread -lraylib -O3

run: main
	./main

dbg: main.c
	gcc -o dbg main.c -Wall -Wextra -lm -lpthread -lraylib -ggdb

valgrind: dbg
	valgrind ./dbg

clean:
	rm main
	rm dbg

