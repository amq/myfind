[![build](https://img.shields.io/travis/amq/myfind.svg)](https://travis-ci.org/amq/myfind) [![coverage](https://img.shields.io/codecov/c/github/amq/myfind.svg)](https://codecov.io/github/amq/myfind)

- the goal is to fully mimic GNU `find` (with a limited set of options)
- performance is comparable to `find`
- no memory leaks
- `lstat` calls are done exactly once per entry
- an unlimited path and symlink length is supported
- "microservices": everyting is decoupled into functions
- exclusively `EXIT_SUCCESS` and `EXIT_FAILURE`
- errors are checked for every function, even `printf`
- the output of `pwd` and `grp` is cached (this makes `-ls` 3x faster)
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
-print              print entries with paths
-ls                 print entry details
-type [bcdpfls]     entries of a specific type
-nouser             entries not belonging to a user
-user <name>|<uid>  entries belonging to a user
-name <pattern>     entry names matching a pattern
-path <pattern>     entry paths (incl. names) matching a pattern
```
