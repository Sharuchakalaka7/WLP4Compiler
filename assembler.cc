#include <iostream>

#include "assembler.h"
using namespace std;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
// ~~~~~~~~~~~~~~~~~~~~ MAIN ASSEMBLERS METHOD (PUBLIC) ~~~~~~~~~~~~~~~~~~~ //
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

Assembler::Assembler(const vector<vector<Token>> &program) : program(program), symbolTable() {}

// Responsible for coordinating the entire assembler's operation
vector<int64_t> Assembler::assemble() {
	vector<int64_t> binary;
	try {
		checkPass();
		codeGenPass(binary);
	} catch (AssemblerException &e) {
		throw e;
	}
	return binary;
}




// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ MAJOR METHODS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// First pass through the program - initialize symbol table, containing each
// 		label-address pair, and some error checking
void Assembler::checkPass() {
	pc = 0;
	bool hasInst;

	for (auto &tokLine : program) {
		hasInst = false;

		for (auto &tok : tokLine) {
			if (tok.getKind() == Token::LABEL) {
				if (symbolTable.count(tok.getLexeme()) > 0)
					throw AssemblerException(ET::DuplicateLabel, tok.getLexeme());
				symbolTable.emplace(tok.getLexeme(), pc);
			} else {
				hasInst = true;
				break;
			}
		}
		if (hasInst) ++pc;
	}
}

// Second pass through the program - generate corresponding machine code binary for
// 		each instruction supplied (using the symbol table for label management), and
//		the remaining error checking cases
void Assembler::codeGenPass(vector<int64_t> &binary) {
	pc = 0;

	// construct corresponding binary instruction from each program line
	for (auto &tokLine : program) {
		unsigned int i = 0;

		// 1. handle label declarations token, if any present
		while (i < tokLine.size() && tokLine[i].getKind() == Token::LABEL) ++i;

		// 2. handle instruction tokens by instruction format, if any present
		if (i < tokLine.size()) {
			++pc;
			try {
				binary.push_back(buildInstruction(tokLine, i));
			} catch (AssemblerException &e) {
				throw e;
			}
		}
	}
}




// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ HELPER METHODS ~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

// Match assembly instruction to MIPS language specifications, then return corresponding
// 		constructed binary - this specific function focuses on setting up the format and
//		templates (and error checking) more than 
int64_t Assembler::buildInstruction(const vector<Token>& tokLine, unsigned int i) {
	int64_t inst = 0;

	try {
		if (tokLine[i].getKind() == Token::WORD || tokLine[i].getKind() == Token::ID) {
			// Fetch the instruction's template and format, if instruction is valid
			vector<pair<Token::Kind,int>> format;
			inst = Assembler::setupInstruction(format, Assembler::getOpType(tokLine[i++].getLexeme()));

			// Check instruction validity
			if (format.empty())
				throw AssemblerException(ET::InvalidOpCode, tokLine[i-1].getLexeme());
			if (tokLine.size()-i < format.size())
				throw AssemblerException(ET::MissingTokens);
			if (tokLine.size()-i > format.size())
				throw AssemblerException(ET::TooManyTokens);
			
			// Proceed by starting instruction build, by each token in format specification
			for (unsigned int j = 0; j < format.size(); ++j, ++i) {
				inst = inst | Assembler::buildToken(tokLine[i], format[j].first, format[j].second);
			}

		} else {
			throw AssemblerException(ET::NotOpCode, tokLine[i].getLexeme());
		}

	} catch (AssemblerException &e) {
		e.tagLine(tokLine);
		throw e;
	}
	return inst;
}


// Return corresponding opcode enumeration
Assembler::Op Assembler::getOpType(const string& opcode) {
	if (opcode == ".word")	return Op::dotword;

	if (opcode == "add")	return Op::add;
	if (opcode == "sub")	return Op::sub;
	if (opcode == "slt")	return Op::slt;
	if (opcode == "sltu")	return Op::sltu;

	if (opcode == "mult")	return Op::mult;
	if (opcode == "multu")	return Op::multu;
	if (opcode == "div")	return Op::div;
	if (opcode == "divu")	return Op::divu;

	if (opcode == "mfhi")	return Op::mfhi;
	if (opcode == "mflo")	return Op::mflo;
	if (opcode == "lis")	return Op::lis;

	if (opcode == "jr")		return Op::jr;
	if (opcode == "jalr")	return Op::jalr;

	if (opcode == "beq")	return Op::beq;
	if (opcode == "bne")	return Op::bne;

	if (opcode == "lw")		return Op::lw;
	if (opcode == "sw")		return Op::sw;

	return Op::invalid;
}


// Provide the instruction's format specification sequence and return the base
//		 template of the instruction (opcode, etc)
int64_t Assembler::setupInstruction(vector<pair<Token::Kind,int>> &format, const Assembler::Op op) {

	// Set instruction format
	switch (op) {

		case Op::dotword:
			// FORMAT: .word i
			format.emplace_back(Token::INT, 32);
			break;

		case Op::add:
		case Op::sub:
		case Op::slt:
		case Op::sltu:
			// R FORMAT: [op] $d, $s, $t
			format.emplace_back(Token::REG, 11);
			format.emplace_back(Token::COMMA, 0);
			format.emplace_back(Token::REG, 21);
			format.emplace_back(Token::COMMA, 0);
			format.emplace_back(Token::REG, 16);
			break;

		case Op::mult:
		case Op::multu:
		case Op::div:
		case Op::divu:
			// R FORMAT: [op] $s, $t
			format.emplace_back(Token::REG, 21);
			format.emplace_back(Token::COMMA, 0);
			format.emplace_back(Token::REG, 16);
			break;

		case Op::mfhi:
		case Op::mflo:
		case Op::lis:
			// R FORMAT: [op] $d
			format.emplace_back(Token::REG, 11);
			break;

		case Op::jr:
		case Op::jalr:
			// R FORMAT: [op] $s
			format.emplace_back(Token::REG, 21);
			break;

		case Op::beq:
		case Op::bne:
			// I FORMAT: [op] $s, $t, i
			format.emplace_back(Token::REG, 21);
			format.emplace_back(Token::COMMA, 0);
			format.emplace_back(Token::REG, 16);
			format.emplace_back(Token::COMMA, 0);
			format.emplace_back(Token::INT, 16);
			break;

		case Op::lw:
		case Op::sw:
			// I FORMAT: [op] $t, i($s)
			format.emplace_back(Token::REG, 16);
			format.emplace_back(Token::COMMA, 0);
			format.emplace_back(Token::INT, 16);
			format.emplace_back(Token::LPAREN, 0);
			format.emplace_back(Token::REG, 21);
			format.emplace_back(Token::RPAREN, 0);
			break;
		default:
			break;
	}

	// Set template (specifc opcodes)
	switch (op) {
		case Op::dotword:	return 0;

		case Op::add:		return 0x20;
		case Op::sub:		return 0x22;
		case Op::slt:		return 0x2a;
		case Op::sltu:		return 0x2b;

		case Op::mult:		return 0x18;
		case Op::multu:		return 0x19;
		case Op::div:		return 0x1a;
		case Op::divu:		return 0x1b;
		
		case Op::mfhi:		return 0x10;
		case Op::mflo:		return 0x12;
		case Op::lis:		return 0x14;

		case Op::jr:		return 0x08;
		case Op::jalr:		return 0x09;

		case Op::beq:		return 0x10000000;
		case Op::bne:		return 0x14000000;

		case Op::lw:		return 0x8c000000;
		case Op::sw:		return 0xac000000;

		default:			return 0;
	}
}


int64_t Assembler::buildToken(const Token& tok, const Token::Kind kind, const int offset) {

	if (tok.getKind() != kind) {
		if (kind != Token::INT || (tok.getKind() != Token::HEXINT && tok.getKind() != Token::ID))
			throw AssemblerException(ET::TokenMismatch, tok.getLexeme());
	}

	switch (kind) {
		case Token::REG:	return buildRegister(tok) << offset;
		case Token::INT:	return buildImmediate(tok, offset);
		case Token::COMMA:
		case Token::LPAREN:
		case Token::RPAREN:	return 0;
		default:			throw AssemblerException(ET::SOMETHINGBROKE, tok.getLexeme());
	}
}


// Pre: given REG token; Produce register opcode template
// Check and produce register number for instruction (REG)
int64_t Assembler::buildRegister(const Token& tok) {
	int64_t reg = tok.toNumber();
	
	if (reg < 0 || reg > 31)
		throw AssemblerException(ET::OutOfBoundsReg, tok.getLexeme());
	return reg;
}


// Check and produce immediate value for instruction (INT, HEXINT, [or LABEL, from a3p3])
// --> bc := "bit count" of the immediate mask, default to 16 bits for formal instruction immediate
int64_t Assembler::buildImmediate(const Token& tok, int bc) {

	if (tok.getKind() == Token::ID && symbolTable.count(tok.getLexeme()+":") == 0)
		throw AssemblerException(ET::UndeclaredLabel, tok.getLexeme());

	int64_t mask = 0;
	int64_t imm = (tok.getKind() != Token::ID) ? tok.toNumber() :
				  					 (bc < 32) ? (symbolTable[tok.getLexeme()+":"] - pc) :
				  								 (symbolTable[tok.getLexeme()+":"] << 2);

	for (int i = 0; i < bc; ++i)
		mask = (mask << 1) + 1;

	if ((imm < 0) && (0-imm > ((mask >> 1)+1)))
		throw AssemblerException(ET::OutOfBoundsImm, tok.getLexeme());
	if (imm > mask)
		throw AssemblerException(ET::OutOfBoundsImm, tok.getLexeme());
	return (imm & mask);
}




// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ERROR HANDLING ~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

AssemblerException::AssemblerException(ET::ErrorType errType, string str) {
	msg = "ERROR: ";
	switch (errType) {
		case ET::NotOpCode:			msg += "Not an operation -";			break;
		case ET::InvalidOpCode:		msg += "Invalid MIPS instruction -";	break;

		case ET::MissingTokens:		msg += "Missing instruction operands";	break;
		case ET::TooManyTokens:		msg += "Too many instruction operands";	break;
		case ET::TokenMismatch:		msg += "Unexpected token found -";		break;

		case ET::OutOfBoundsImm:	msg += "Immediate is out of bounds -"; 	break;
		case ET::OutOfBoundsReg:	msg += "Invalid register number -"; 	break;

		case ET::UndeclaredLabel:	msg += "Label was not declared -";		break;
		case ET::DuplicateLabel:	msg += "Label already declared -";		break;

		default:					msg += "*SOMETHING* BROKE... -";
	}

	if (str != "") msg += " \"" + str + "\"";
}

void AssemblerException::tagLine(const vector<Token> &tokLine) {
	msg += "\n\t ==> ";
	for (auto &tok : tokLine)
		msg += tok.getLexeme() + " ";
}

const string& AssemblerException::what() const { return msg; }
