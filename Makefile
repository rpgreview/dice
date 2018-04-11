.PHONY: all check clean

all: dice

dice:
	gcc -O2 -D_FORTIFY_SOURCE -Wl,-z,relro -Wl,-z,now -Wall -flto -fopenmp -o dice -lm -lreadline -lncurses dice.c interactive.c parse.c

check: dice
	./tests/dice-test.sh

clean:
	rm -vf ./dice
