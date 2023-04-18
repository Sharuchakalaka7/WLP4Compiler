# WLP4Compiler #

WLP4 is a restricted subset of the modern C language, almost similar to its predecessor language, B. WLP4 was designed to study the compilation process on an introductory level, to be able to implement numbers, pointers, procedures, and basic arithmetic and usage. The assembly language used alongside the process is the MIPS language (or at least a restricted subset of it).

The code in this repository altogether make the compiler. Currently, the repository has:
* `asm.cc`, `assembler.cc`, and `scanner.cc` - the immediate assembler from MIPS assembly to machine code
* `wlp4scan.cc` - the scanner/lexer that tokenizes the raw WLP4 source code
* `wlp4parse.cc` - the parser, which builds a parse tree using the lexer tokens and WLP4 grammar specifications
* `wlp4type.cc` - the context-free analysis tool, catching any semantic errors and in the process annotates the parse tree with type info
* `wlp4gen.cc` - the code generator, producing the equivalent MIPS assembly code for the program

Each bolded _filename_ file above can be compiled with a C++ compiler. For instance, with `g++`:

	g++ -std=c++17 filename.cc -o filename

Then to convert a WLP4 source code to MIPS assembly, simply run:

	./wlp4scan.cc < src.wlp4 | ./wlp4parse | ./wlp4type | ./wlp4gen

There are still several more pieces to include - the linker, loader, and the actual actual runner. I will work on updating that. Secondly, I will also try organizing all the programs more logically separating each crucial part of the process, then combine it all into one exectuable that is the _true_ compiler. Lastly, there are many optimizations that can be included for the generated code. So far, only three optimizations are fully implemented.

This assembler, compiler, and all supporting programs were developed throughout the term of taking CS 241 at the University of Waterloo. Credits for the `wlp4data.h` CFG details go to the course staff of CS 241 Winter 2023 and earlier.

If you have any ideas or suggestions on how else to expand, please let me know through email at <ahmed.shahriar343@gmail.com>.
