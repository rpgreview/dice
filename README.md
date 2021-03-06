Dice
====

A multi-threaded interpreter and interactive shell for standard dice notation such as 3d6 or d4 + 2.

For usage, see the included manual dice(1).

For installation, see INSTALL.

For attribution and legalities see AUTHORS and LICENSE.



Usage
----

```
Usage: dice [-?hV] [-p STRING] [-s NUMBER] [--prompt=STRING] [--seed=NUMBER]
            [--help] [--help] [--usage] [--version] [file]
Dice -- An interpreter for standard dice notation (qv Wikipedia:Dice_notation)

  -p, --prompt=STRING        Set the dice interactive prompt to STRING.
                             (Default: 'dice> ')
  -s, --seed=NUMBER          Set the seed to NUMBER. (Default is based on
                             current time.)
  -?, --help                 Give this help list
  -h, --help                 Print this help message.
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.
```

### Notation

In addition to standard dice notation,
you can prefix any dice command with `<number> x` to make multiple rolls at once.
Example:

```sh
$ ./dice <<< "5x d6"
5 1 5 5 3
```

All spaces are ignored; comments start with a hash.
Therefore, while the following looks a little strange, it is syntactically valid:

```
dice> 5xd6#This rolls a d6 five times.
```

You can make multiple different rolls on a single line delimited by semi-colons:

```
dice> d100; d20; d6+2 # This is a useful pattern for RuneQuest: roll to hit, hit location, damage.
17
20
8
```

You can arithmetically combine multiple dice types and constants in an intuitive manner:

```
dice> 1+2+3+4
10
dice> 4x-1-2-3-4
-10 -10 -10 -10
dice> d6+d4
6
dice> d2-1+d2-1
1
```

You can use an exclamation mark for "exploding" dice, ala Cyberpunk 2020 or older editions of Shadowrun:

```
dice> 10x d6!
4 1 1 11 3 3 1 9 3 4
```

The exploding dice operator binds to the `dX` part of `NdX` notation.
I.e., `3d6!` means roll a `1d6!` three times and add the results together.
It doesn't mean "roll a 3d6 and keep adding more to the sum so long as you keep getting 18s".


You can use a `k` followed by a number to denote _keeping_ that number of dice (i.e. discarding the lowest dice).
Eg `4d6k3` means roll `4x d6`, keep the best three, discarding the lowest.
(This is a common means of generating character attributes between 3 and 18, skewed so that the median is somewhat higher than the "human average" of about 10 or 11.)
Example:

```
dice> 8x 4d6k3
12 12 8 12 15 14 16 14
```

It is possible to apply "keeping" to exploding dice (but not vice versa):

```
dice> 8x 4d6!k3
10 13 18 19 15 13 12 9
dice> 8x 4d6k3!
Cannot process operator '!' while in state 'check_more_rolls'
```

You case use a `T` followed by a number to denote the threshold on a dice pool.
A single roll with a threshold will return 1 if the result was greater than or equal to the threshold, otherwise it will return 0.
Thresholding changes the usual behaviour of the `x` operator:
a repeated roll subject to a threshold will return the number of successes, not a sequence of 0s and 1s.

Example from _Cyberpunk 2020_ where a Solo with Intelligence 6 and Awareness/Notice 7 succeeds at an average perception check:

```
dice> d10! + 6 + 7 T 15
1
```

Note the exclamation mark (`!`), since _Cyberpunk 2020_ combines both penetration rolls and thresholds.

Example from _Mage: The Ascension (2E)_ where a character with an Arete of 6 gets five successes on a coincidental one-dot spell:

```
dice> 6x d10 T4
5
```

In practice, using the threshold operator only has limited value, as systems where thresholding is used often also have a notion of "critical failure".
For example, in _Cyberpunk 2020_, any `d10` roll that comes up `1` implies a fumble.

In such systems, it is required to know the raw dice rolls; it is insufficient to simply make a naive "success test".
The variety of systems contraindicates any non-trivial thresholding implementation.


### Modes

Similar to common shells such as Bash and Dash, Dice offers various modes of operation, including scripted vs interactive use, or reading from `stdin` as part of a pipeline.

#### Pipeline

```sh
$ echo "5x d6" | dice | xargs -n1
4
6
1
5
4
$ for sides in 6 8 10; do dice <<< "5x d$sides" | xargs -n1 | datamash sum 1; done
20
31
27
```


#### Scripted

```sh
$ cat > /tmp/testing.dice << EOF
> #! /bin/dice
>
> 3d6
> 5x 7d8 + 23
> EOF
$ chmod +x /tmp/testing.dice
$ /tmp/testing.dice
12
51 58 60 57 55
```


#### Interactive

```sh
$ ./dice
dice> 3d6
6
dice> 5x 7d8 + 23
54 42 53 52 50
dice> quit
```

You can even use this program as your login shell if you wish:

```sh
$ sudo useradd -m -s /bin/dice dicey
$ sudo passwd dicey
New password:
Retype new password:
passwd: password updated successfully
$ su - dicey
Password:
dice> d3
1
dice> 44 x d6
6 2 5 3 6 5 2 1 4 4 6 5 3 5 3 3 6 6 1 1 2 1 5 2 4 2 2 5 5 3 1 5 4 5 1 4 4 3 4 2 6 4 6 3
dice> quit
$
```

Interactive use incorporates the [GNU readline library](https://tiswww.case.edu/php/chet/readline/rltop.html).
This means you can customise the prompt, even with ANSI-escape style colours,
and cycle through history using the up and down arrow keys.

Between sessions, history is stored in `~/.dice_history`.


Installation
----

```sh
$ make
$ make check # optional
$ ln -svf $PWD/dice ~/bin/
```

### Dependencies

Check the output of `grep -h '#include <' *.[ch] | sort -u`.

Most dependencies will be installed on most Linuces but maybe you will need your
package manager to install things like termcap, wordexp or OpenMP.

Note, however, that GCC ships with libgomp, so explicit installation of OpenMP probably won't be required.
Not all distros are developer-friendly, so you might not have GCC by default.
Eg, you might need to install the `build-essential` package if you are on Ubuntu.


Validity checks
----

The software will try to warn when the requested rolls could cumulatively cause an integer overflow. For example:

```
dice> 1000000d100000000 + 9223372036854775805
Warning: +1000000d100000000+9223372036854775805d1 are prone to integer overflow.
-9223322044654812424
```

However, for exploding dice, there is no upper bound on the possible results.
The number of "explosions" will be a geometrically distributed random variable[1], so although large outcomes may have negligible probability for any single dice, they are nevertheless possible.
Rather than attempt the impossible, Dice ignores the prospect of explosions when validating its input.

For most games, the number of dice rolled is fairly small, so the risk of integer overflow from exploding dice can just be ignored.
If you are perverse enough to ask for a very large number of exploding dice, then the results are on your own head.

[1] https://en.wikipedia.org/wiki/Geometric_distribution


Contributing
----

Discussion and pull requests covering the following would be welcome:

* Bug fixes
* Performance enhancements
* Sensible extensions to the syntax (sensible = it should be useful and have a clear meaning)
* Cleaner and more robust methods for parsing the syntax (I am a bit of a noob at writing parsers in C.)
* Establishing a `~/.dicerc` file or such for things like customising the prompt, controlling the behaviour of history, etc.

If you raise an issue about a bug, please consider preparing a pull request which will add a test for the bug in the `./tests/` directory.
