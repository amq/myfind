[![build](https://img.shields.io/travis/amq/myfind.svg)](https://travis-ci.org/amq/myfind) [![coverage](https://img.shields.io/codecov/c/github/amq/myfind.svg)](https://codecov.io/github/amq/myfind)

- the goal is to fully mimic GNU `find` (with a limited set of options)
- performance is comparable to `find`
- no memory leaks
- no action is repeated, if possible. Most notably, `lstat` calls are done exactly once per entry
- no limits for the path and symlink length
- "microservices": everyting is decoupled into functions
- relying exclusively on `EXIT_SUCCESS` and `EXIT_FAILURE`
- errors are checked for every function, even `printf`
- the output of `pwd` and `grp` is cached (this makes `-ls` 3x faster)
- constant testing during development: performance, memory usage, static code analysis, Travis with `gcc` and `clang`
- consistent code formatting (LLVM), automatically maintained by `clang-format`
- full support of `gnu99` and `gnu11` standards

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
./myfind <file or directory> [ <aktion> ]
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
# tested 5 times and recorded the best result

time ./myfind / -ls > mylist.txt

real	0m1.464s
user	0m0.604s
sys	    0m0.844s

VIRT    RES    SHR
21868   2512   2188


time find / -ls > list.txt

real	0m1.519s
user	0m0.784s
sys 	0m0.728s

VIRT    RES    SHR
25044   2972   2568
```