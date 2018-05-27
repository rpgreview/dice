.PHONY: all check clean

all: dice

dice:
	gcc -O2 -D_FORTIFY_SOURCE -Wl,-z,relro -Wl,-z,now -Wall -flto -fopenmp -o dice -lm -lreadline -lncurses dice.c io.c parse.c roll-engine.c

check: dice
	./tests/testing.dice
	./tests/dice-test.sh

clean:
	rm -vf ./dice
