# Cobweb C standard utilities

Cobweb is a C library that takes inspiration from [nob.h](https://github.com/tsoding/nob.h), and is designed to be used without a build-system.
That is, the utilities in this library permit compiling C projects using C source files as build-recipes.

## Requirements

This is a UNIX only library, meaning there are no guarantees that any of the functions defined in this library
will work as intended or at all on unsupported operating systems.

The current build has been configured to use `gcc` as the C compiler, however it should be near trivial to reconfigure
it to use `clang`.

## Building

The C-based build system is a work in progress, and there is currently no user-friendly method to bootstrap the build process. The current
method for initially compiling the build recipe (E.g "bootstrapping") is to compile using a manual GCC invocation:
```sh
gcc -o build src/*.c src/build-system/*.c
```
Following this, the build executable is fully capable of recompiling the project (including itself) on it's own. The arguments of this
executable can be retrieved by running it without any arguments, or by explicitly passing the `--help` flag:
```sh
# Either:
./build
# Or:
./build --help
```

