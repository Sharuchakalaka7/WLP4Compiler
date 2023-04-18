#ifndef ASSEMBLER_HEADER
#define ASSEMBLER_HEADER

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include "scanner.h"


// Forward declare
class ET;


// Assembler class definition
class Assembler {
	// Data Members
	int64_t pc;
	const std::vector<std::vector<Token>> &program;
	std::map<std::string,int64_t> symbolTable;
	enum Op : int {
		invalid=0,
		dotword,
		add, sub, slt, sltu,
		mult, multu, div, divu,
		mfhi, mflo, lis,
		jr, jalr,
		lw, sw,
		beq, bne,
	};

	// Helper Methods
	int64_t buildInstruction(const std::vector<Token>&, unsigned int);
	Op getOpType(const std::string&);
	int64_t setupInstruction(std::vector<std::pair<Token::Kind,int>>&, const Op);
	int64_t buildToken(const Token&, const Token::Kind, const int);
	int64_t buildRegister(const Token&);
	int64_t buildImmediate(const Token&, int);

	// Major Methods
	void checkPass();
	void codeGenPass(std::vector<int64_t>&);

  public:
	Assembler(const std::vector<std::vector<Token>>&);
	std::vector<int64_t> assemble();
};

// Error Types
class ET {
  public:
	enum ErrorType : int {
			SOMETHINGBROKE = 0,
		// opcode errors
			NotOpCode,
			InvalidOpCode,
		// token errors
			MissingTokens,
			TooManyTokens,
			TokenMismatch,
		// out of bounds errors
			OutOfBoundsReg,
			OutOfBoundsImm,
		// label errors
			UndeclaredLabel,
			DuplicateLabel,
	};
};

// Define general assembler exception class
class AssemblerException {
	std::string msg;
  public:
	AssemblerException(ET::ErrorType, std::string str = "");
	void tagLine(const std::vector<Token>&);
	const std::string &what() const;
};

#endif
