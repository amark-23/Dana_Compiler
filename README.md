# Dana Compiler

This repository contains the Dana Compiler, which includes a lexer and parser implemented using Flex and Bison. The Makefile automates the build process, allowing seamless compilation, testing, and cleanup.

## Cloning the Repository
To get started, clone this repository using:
```sh
git clone --recurse-submodules https://github.com/amark-23/Dana_Compiler.git
cd Dana_Compiler
```

## Building the Compiler
Navigate to root directory and run:
```sh
make
```
This will generate the necessary files, compile the lexer and parser, and create the `dana` executable.

## Running Tests
To test the compiler using the `.dana` files located in the submodule's directory, run:
```sh
make test
```
This will execute the `dana` compiler on each `.dana` test file and display the results.

## Cleaning Up
To remove all generated files except the original source files, use:
```sh
make distclean
```
This will clean up all compiled objects and executables, leaving only the original source files intact.

## Dependencies
Ensure you have the following tools installed:
- `flex` (for lexical analysis)
- `bison` (for syntax analysis)
- `gcc` (for compilation)
- `llvm-config` (for code generation, version: 18.1.3)

## Author

Developed by [amark-23](https://github.com/amark-23) | [gtiso](https://github.com/gtiso).

