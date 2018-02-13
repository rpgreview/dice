Dice
====

An interpreter and interactive shell for standard dice notation such as `3d6` or `d4 + 2`.

You can use this as your login shell if you wish:

~~~{.sh}
$ sudo useradd -m -s /bin/dice dicey
$ sudo passwd dicey
New password:
Retype new password:
passwd: password updated successfully
$ su - dicey
Password:
> d3
1
> d3
2
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
