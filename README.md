[![build](https://img.shields.io/travis/amq/myfind.svg)](https://travis-ci.org/amq/myfind) [![coverage](https://img.shields.io/codecov/c/github/amq/myfind.svg)](https://codecov.io/github/amq/myfind)

- the goal is to fully mimic GNU `find` (with a limited set of options)
- performance is comparable to `find`
- no memory leaks
- doing `lstat` calls exactly once per entry
- supporting an unlimited path and symlink length
- building upon "microservices", everyting is decoupled into functions
- using exclusively `EXIT_SUCCESS` and `EXIT_FAILURE` for int functions
- checking for errors for every function, even `printf`
- caching the output of `pwd` and `grp` (this makes `-ls` 3x faster)
- constant testing during development: performance, memory usage, static code analysis, Travis with `gcc` and `clang`
- consistent code style (LLVM), automatically maintained by `clang-format`

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
-user <name>|<uid>  entries belonging to a user
-name <pattern>     entry names matching a pattern
-type [bcdpfls]     entries of a specific type
-print              print entries with paths
-ls                 print entry details
-nouser             entries not belonging to a user
-path               entry paths (incl. names) matching a pattern
```
