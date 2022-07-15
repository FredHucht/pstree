# pstree
## Introduction
`pstree` is a small program that shows the process listing (`ps`) as a tree (as the name implies...). It has several options to make selection criteria and to change the output style.

It should compile under most Un*xes, tested are AIX, Linux, HP-UX, A/UX, SunOS, Solaris, (Free|Open|Net)BSD, MacOSX/Darwin/macOS, and many others.

Under AIX & Linux, pstree directly reads the process table using `getproc()`/`getuser()` or the `/proc` file system. Under all other Un*xes, `pstree` reads the output of `/bin/ps`.

## Compilation

Take an ANSI C compiler, e.g., `gcc`, and just enter

    $ [g]cc -O -o pstree pstree.c

Alternatively, enter `make`.

## Installation

Put the binary `pstree` into appropriate bindir, e.g., `/usr/local/bin`.

Optionally, put the manpage `pstree.1` to, e.g., `/usr/local/share/man/man1`.

## Changes

For changes up to v2.40, see end of `pstree.c`.
   
## Usage

~~~
$ ./pstree -?
pstree $Revision: 2.40 $ by Fred Hucht (C) 1992-2022
EMail: fred AT thp.uni-due.de

Usage: pstree [-f file] [-g n] [-l n] [-u user] [-U] [-s string] [-p pid] [-w] [pid ...]
   -f file   read input from <file> (- is stdin) instead of running
             "ps -eo uid,pid,ppid,pgid,args"
   -g n      use graphics chars for tree. n=1: IBM-850, n=2: VT100, n=3: UTF-8
   -l n      print tree to n level deep
   -u user   show only branches containing processes of <user>
   -U        don't show branches containing only root processes
   -s string show only branches containing process with <string> in commandline
   -p pid    show only branches containing process <pid>
   -w        wide output, not truncated to window width
   pid ...   process ids to start from, default is 1 (probably init)

Process group leaders are marked with '='.
~~~

## History

`pstree` started as a shell script back in the 90s, the first C version is from 1992. Since 1994, the source was managed using RCS. Since 2022, it lives here on GitHub.

`pstree` was formerly available under 
* `ftp://ftp.thp.uni-due.de/pub/source/` (server down)
* `http://www.thp.uni-due.de/pstree/` (server down)

Have fun, Fred
