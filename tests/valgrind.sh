#! /bin/bash

hash valgrind 2> /dev/null \
    || {
        1>&2 echo "Valgrind not found"
        ./dice ./tests/testing.dice
    }
hash valgrind 2> /dev/null \
    && valgrind -v --tool=memcheck --leak-check=full --show-leak-kinds=all --show-reachable=no --track-origins=yes \
    ./dice <<< "5x d4 + 2 + d6; 4d6!k3; 10x d2 T2;"
