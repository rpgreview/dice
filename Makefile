.PHONY: all check

all: dice

dice:
	gcc -Wall -o dice -lm -lreadline dice.c

check: dice
	./tests/dice-test.sh
