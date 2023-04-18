#include <iostream>
#include <string>
#include <vector>
#include "scanner.h"
#include "assembler.h"
using namespace std;

/*
 * C++ Starter code for CS241 A3
 *
 * This file contains the main function of your program. By default, it just
 * prints the scanned list of tokens back to standard output.
 */
int main() {
	string line;
	try {
		#ifdef PRINT_TOKENS
			while (getline(cin, line)) {
				// This example code just prints the scanned tokens on each line.
				vector<Token> tokenLine = scan(line);

				// This code is just an example - you don't have to use a range-based
				// for loop in your actual assembler. You might find it easier to
				// use an index-based loop like this:
				// for(int i=0; i<tokenLine.size(); i++) { ... }
				for (auto &token : tokenLine) {
					cout << token << ' ';
				}

				// Remove this when you're done playing with the example program!
				// Printing a random newline character as part of your machine code
				// output will cause you to fail the Marmoset tests.
				cout << endl;
			}
		#else
			// Actual assembler output goes here
			vector<vector<Token>> program;

			// read and construct each line into the above program format
			while (getline(cin, line)) {
				vector<Token> tokenLine = scan(line);
				// ignore empty lines
				if (!tokenLine.empty())
					program.push_back(tokenLine);
			}

			// Assemble program and print the resulting binary to stdout, as ASCII characters
			// 		(each instruction contains 32/8=4 such characters)
			Assembler assembler(program);
			for (auto &inst : assembler.assemble())
				cout << ((char) (inst >> 24)) << ((char) (inst >> 16)) << ((char) (inst >> 8)) << ((char) inst);

		#endif
	} catch (ScanningFailure &f) {
		cerr << f.what() << endl;
		return 1;
	} catch (AssemblerException &e) {
		cerr << e.what() << endl;
		return 2;
	}

	// You can add your own catch clause(s) for other kinds of errors.
	// Throwing exceptions and catching them is the recommended way to
	// handle errors and terminate the program cleanly in C++. Do not
	// use the std::exit function, which may leak memory.

	return 0;
}
