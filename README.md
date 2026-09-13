# Cobweb C standard utilities

Cobweb is a C library that takes inspiration from [nob.h](https://github.com/tsoding/nob.h), and is designed to be used without a build-system.
That is, the utilities in this library permit compiling C projects using C source files as build-recipes.

## Requirements

This is a UNIX only library, meaning there are no guarantees that any of the functions defined in this library
will work as intended or at all on unsupported operating systems.

The current build has been configured to use `gcc` as the C compiler, however it should be near trivial to reconfigure
it to use `clang`.

## Building

```sh
./bootstrap-build
./build
./build
```

Since this project uses C source files as build-recipes, the recipe itself has to be compiled before it can be run.
This can be done by either using the provided `bootstrap-build` executable, or by manually compiling the build recipe.

The `bootstrap-build` file assumes your system has a default C compiler installed named `cc` and will compile the
build recipe using the following compiler flags:

- `-DBOOTSTRAP_BUILD=1`
- `-Isrc`
- `-o build`

`-DBOOTSTRAP_BUILD=1` is optional, but recommended as it helps set up some pre-build actions before the actual build
starts.

When the build recipe is compiled, run it as `./build`. This will cause the build recipe to configure some initial
repository state, and then recompile itself.

> [!NOTE]
> Running `./build` is only required if the `BOOTSTRAP_BUILD` preprocessor symbol was defined in the initial build.

When the build recipe is compiled and configured, running it again will begin the compilation. Optionally, the build
recipe also supports the following flags:

- `--self` recompiles the build recipe
- `--test` runs tests after performing the compilation
- `-j <number>` specify the number of parallel compilation jobs



