Dice
====

An interpreter and interactive shell for standard dice notation such as `3d6` or `d4 + 2`.


Usage
----

~~~
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
~~~

### Notation

In addition to standard dice notation,
you can prefix any dice command with `<number> x` to make multiple rolls at once.
Example:

~~~{.sh}
$ ./dice <<< "5x d6"
5 1 5 5 3
~~~

All spaces are ignored; comments start with a hash.
Therefore, while the following looks a little strange, it is syntactically valid:

~~~
dice> 5xd6#This rolls a d6 five times.
~~~


### Modes

This program works in a similar way to standard Unix shells.
It has various modes of operation, such as scripted vs interactive use.

#### Scripted

~~~{.sh}
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
~~~


#### Interactive

~~~{.sh}
$ ./dice
dice> 3d6
6
dice> 5x 7d8 + 23
54 42 53 52 50
dice> quit
~~~

You can even use this program as your login shell if you wish:

~~~{.sh}
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
~~~


Installation
----

Although this program does basic lexing and parsing,
a self-hosted compiler from dice notation to x86 assembly is still some way in the future.

For now you still have to use standard Unix tools:

~~~{.sh}
$ make
$ ln -svf $PWD/dice ~/bin/
~~~
