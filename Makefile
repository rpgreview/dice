.PHONY: all

all:
	gcc -Wall -o dice -lm -lreadline dice.c
