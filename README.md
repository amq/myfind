[![build](https://img.shields.io/travis/amq/myfind.svg)](https://travis-ci.org/amq/myfind) [![coverage](https://img.shields.io/codecov/c/github/amq/myfind.svg)](https://codecov.io/github/amq/myfind) [![analysis](https://img.shields.io/coverity/scan/8222.svg)](https://scan.coverity.com/projects/amq-myfind)

- the goal is to fully mimic GNU `find` (with a limited set of options)
- performance is comparable to `find`
- no memory leaks
- no action is repeated, if possible. Most notably, `lstat` calls are done exactly once per entry
- no limits for the path and symlink length
- "microservices": everyting is decoupled into functions
- relying exclusively on `EXIT_SUCCESS` and `EXIT_FAILURE`
- errors are checked for every function, even `printf`
- the output of `pwd` and `grp` is cached (this makes `-ls` 3x faster)
- constant testing during development: performance, memory usage, Travis with `gcc` and `clang`
- using static code analysis with `scan-build` and `coverity`
- consistent code formatting (LLVM), automatically maintained by `clang-format`
- full support of the `gnu99` and `gnu11` standards

Building
```
git clone https://github.com/amq/myfind.git
cd myfind
mkdir -p build
cd build
cmake ..
make
```

Usage
```
./myfind [ <location> ] [ <aktion> ]
-help               show this message
-print              print entries with paths
-ls                 print entry details
-type [bcdpfls]     entries of a specific type
-nouser             entries not belonging to a user
-user <name>|<uid>  entries belonging to a user
-name <pattern>     entry names matching a pattern
-path <pattern>     entry paths (incl. names) matching a pattern
```

Performance
```
# tested 5 times and recorded the fastest time and the worst memory case

time ./myfind / -ls > mylist.txt
real	0m1.421s
user	0m0.732s
sys	    0m0.684s
VIRT    RES    SHR
21752   2808   2452

time find / -ls > list.txt
real	0m1.495s
user	0m0.784s
sys	    0m0.696s
VIRT    RES    SHR
25196   3380   2952


time ./myfind / -user 1000 -ls > mylist.txt
real	0m0.752s
user	0m0.180s
sys	    0m0.568s
VIRT    RES    SHR
21816   2824   2452

time find / -user 1000 -ls > list.txt
real	0m0.657s
user	0m0.232s
sys	    0m0.420s
VIRT    RES    SHR
25076   3364   2928


time ./myfind / -name *sys* -ls > mylist.txt
real	0m0.727s
user	0m0.244s
sys	    0m0.480s
VIRT    RES    SHR
21780   2904   2544

time find / -name *sys* -ls > list.txt
real	0m0.460s
user	0m0.180s
sys	    0m0.276s
VIRT    RES    SHR
25400   3604   3008
```
