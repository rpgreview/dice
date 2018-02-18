#! /bin/bash

./dice <<< 3d6
./dice <<< "5x 7d8 + 23"
./dice ./tests/testing.dice
hash valgrind 2> /dev/null \
    && valgrind -v --tool=memcheck --leak-check=full --show-leak-kinds=all --show-reachable=no --track-origins=yes \
    dice ./tests/testing.dice

hash datamash 2>/dev/null \
&& for att in STR DEX CON INT WIS CHA; do
    printf "%s: " $att
    echo "4x d6" | ./dice | xargs -n1 | sort | tail --lines=+2 | datamash sum 1
done
