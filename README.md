# Dana Compiler

This repository contains the Dana Compiler, which includes a lexer and parser implemented using Flex and Bison. The Makefile automates the build process, allowing seamless compilation, testing, and cleanup.

## Cloning the Repository
To get started, clone this repository using:
```sh
git clone https://github.com/amark-23/Dana_Compiler.git
cd Dana_Compiler
```

## Project Structure
```
Dana_Compiler/
│-- src/
│   ├── lexer.l       # Flex file for lexical analysis
│   ├── parser.y      # Bison file for syntax analysis
│   ├── lexer.h       # Header file for lexer-parser integration
│   ├── Makefile      # Build automation file
│-- Dana/             # Directory containing .dana test files
|-- archive/          # Directory containing older versions of lexer and parser
│-- README.md         # Project documentation
```

## Building the Compiler
Navigate to the `src/` directory and run:
```sh
cd src
make
```
This will generate the necessary files, compile the lexer and parser, and create the `dana` executable.

## Running Tests
To test the compiler using the `.dana` files located in the `Dana/` directory, run:
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

## Author
Developed by [amark-23](https://github.com/amark-23) | [gtiso](https://github.com/gtiso).