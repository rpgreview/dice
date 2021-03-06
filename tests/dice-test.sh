#! /bin/bash

./dice <<< 3d6
./dice <<< "5x 7d8 + 23"
./dice <<< "5x d4 + 2 + d6"
./dice <<< ";;;;;;"
./dice <<< "-1-2-3-4"
./dice <<< "4x-1-2-3-4"
./dice <<< 4x-1-2d4-d6-1

hash datamash 2> /dev/null \
    || 1>&2 echo "Datamash not found"
hash datamash 2>/dev/null \
&& {
    echo "Testing pipes in a for-loop:"
    for att in STR DEX CON INT WIS CHA; do
        printf "%s: " $att
        echo "4x d6" | ./dice | xargs -n1 | sort | tail --lines=+2 | datamash sum 1
    done
    echo "Testing semi-colon delimiters from a here-string:"
    for att in STR DEX CON INT WIS CHA; do
        printf "%s: " $att
        ./dice <<< "d6; d6; d6; d6" | sort | tail --lines=+2 | datamash sum 1
    done
}
