Dice
====

A multi-threaded interpreter and interactive shell for standard dice notation such as `3d6` or `d4 + 2`.


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

You can arithmetically combine multiple dice types and constants in an intuitve manner:

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


Contributing
----

Discussion and pull requests covering the following would be welcome:

* Bug fixes
* Performance enhancements
* Sensible extensions to the syntax (sensible = it should be useful and have a clear meaning)
* Cleaner and more robust methods for parsing the syntax (I am a bit of a noob at writing parsers in C.)
* Establishing a `~/.dicerc` file or such for things like customising the prompt, controlling the behaviour of history, etc.

If you raise an issue about a bug, please consider preparing a pull request which will add a test for the bug in the `./tests/` directory.
