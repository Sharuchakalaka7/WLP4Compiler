#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include "wlp4data.h"




class CFG {
	// Class CFG defines Context Free Grammars as known and used usually. Consists of:
	// - the start symbol non-terminal symbol
	// - the list of production rules, numbered by each natural number
  protected:
	class Production {
		// Class Production defines and stores a production rule and all its data:
		// - the left hand non-terminal for which the rule is defined
		// - the sequence of terminals/non-terminals that define the rule
		int ntCount;
		std::string nt;
		std::vector<std::string> rule;
	  public:
		Production(int c, std::string nt, std::vector<std::string> &rule) : ntCount(c), nt{nt}, rule(rule) {}

		/** Only have simple production rule accessor methods **/
		const std::string &getNT() { return nt; }

		int getAllCount() { return ((int) rule.size()); }			// total symbol count in rule

		int getNTCount() { return ntCount; }				// number of non-terminals in rule

		int getTCount() { return (((int) rule.size()) - ntCount); }	// number of terminals in rule

		const std::vector<std::string> &getRule() { return rule; }
	};

	std::string start;
	std::vector<Production> prods;
	std::set<std::string> terminals;
	std::set<std::string> nonTerminals;
  public:
	CFG() : start{""}, prods(), terminals(), nonTerminals() {}

	/** Extend production rule accessor methods by pinpointing numbering **/
	const std::string &getProdNT(int n) {
		return prods[n].getNT();
	}

	int getProdAllCount(int n) {
		return prods[n].getAllCount();
	}

	int getProdNTCount(int n) {
		return prods[n].getNTCount();
	}

	int getProdTCount(int n) {
		return prods[n].getTCount();
	}

	const std::vector<std::string> &getProdRule(int n) {
		return prods[n].getRule();
	}

	/** Terminal or non-terminal checks **/
	bool isNonTerminal(std::string &sym) {
		return (nonTerminals.count(sym) != 0);
	}

	bool isTerminal(std::string &sym) {
		return (terminals.count(sym) != 0);
	}

	/** Add a new production rule **/
	void addProd(std::string &nt, std::vector<std::string> &rule) {
		if (prods.size() == 0) start = nt;
		int ntCount = 0;
		
		// count nt as a non-terminal
		if (!isNonTerminal(nt)) {
			if (isTerminal(nt)) terminals.erase(terminals.find(nt));
			nonTerminals.insert(nt);
		}
		// count each terminal in rule, then make production
		for (std::string &r : rule) {
			if (isNonTerminal(r)) 	++ntCount;
			else 					terminals.insert(r);
		}
		prods.push_back(Production(ntCount, nt, rule));
	}
};




class WLP4ParseTree {
	// Represents a WLP4ParseTree with the option to annotate if needed

	/** Internal parse tree structure **/
	struct Node {
		std::string kind;				// token kind (if terminal) or rule owner (if non-terminal)
		std::string seq;				// sequence of terminals and non-terminals (including kind)
		std::string type;				// annotated type, if seq represents an expression
		std::vector<Node*> children;
		Node(std::string &kind, std::string seq) : kind(kind), seq(seq), type(TYPE_NONE), children() {}
		~Node() { for (Node *c : children) delete c; }
	};

	/** Internal data for individual variables in procedures **/
	struct VarData {
		int loc;
		std::string &type;
		std::string TEMP = "";
		VarData() : loc(0), type(TEMP) {};
		VarData(int loc, std::string &type) : loc(loc), type(type) {}
	};

	/** Internal data for individual procedures **/
	struct ProcData {
		std::string id;
		std::map<std::string,VarData> symTable;		// number of declarations+params in proc is symTable.size()

		ProcData() : id(""), symTable() {}
		ProcData(std::string &id) : id(id), symTable() {}
		VarData &operator[](std::string &varID) { return symTable[varID]; }
	};




	/** recursive IO functions for the trees **/
	Node *readTree(std::istream &in, std::string procID = "wain") {
		std::string str;
		Node *node = nullptr;

		if (getline(in, str)) {
			std::string kind, seq, word;
			std::istringstream iss(str);
			std::vector<Node*> children;

			iss >> kind;
			seq = kind;
			bool kindIsTerminal = cfg.isTerminal(kind);
			// if type information given, then loop ends with word = ":"
			while (iss >> word && word != DIR_EMPTY && word != ":") {
				seq += " " + word;
				if (kindIsTerminal) continue;

				Node *child = readTree(in, procID);
				if (child == nullptr) continue;
				children.push_back(child);
			}

			node = new Node(kind, seq);
			if (word == ":" && iss >> word) node->type = word;
			node->children = std::move(children);
		}
		return node;
	}




	/** initialize procedures table and all symbol tables within **/

	/** initialize procedure table for each procedure **/
	void initptable(Node *node) {
		if (node == nullptr) return;

		// start → BOF procedures EOF
		if (node->kind == "start") {
			initptable(node->children[1]);

		// procedures → main
		// procedures → procedure procedures
		} else if (node->kind == "procedures") {
			initptable(node->children[0]);
			if (node->children.size() > 1)
				initptable(node->children[1]);

		// main → INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
		// procedure → INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
		} else {
			std::string procID;
			std::istringstream(node->children[1]->seq) >> procID >> procID;
			ProcData &table = ptable[procID] = ProcData(procID);

			if (procID == "wain") {
				initsymtable(node->children[3], table);
				initsymtable(node->children[5], table);
				initsymtable_dcls(node->children[8], table);

			} else {
				initsymtable_params(node->children[3], table);
				initsymtable_dcls(node->children[6], table);
			}
		}
	}

	/** initialize parameters from procedures in given symbol table **/
	void initsymtable_params(Node *node, ProcData &table) {
		// params → ε
		// params → paramlist
		if (node->kind == "params") {
			if (node->children.size() > 0)
				initsymtable_params(node->children[0], table);

		// paramlist → dcl
		// paramlist → dcl COMMA paramlist
		} else {
			initsymtable(node->children[0], table);
			if (node->children.size() > 1)
				initsymtable_params(node->children[2], table);
		}
	}

	/** initialize declarations from procedures in given symbol table **/
	void initsymtable_dcls(Node *node, ProcData &table) {
		// dcls → ε
		// dcls → dcls dcl BECOMES NUM SEMI
		if (node->children.size() > 0) {
			initsymtable_dcls(node->children[0], table);
			initsymtable(node->children[1], table);
		}
	}

	/** initialize variable in given symbol table **/
	void initsymtable(Node *node, ProcData &table) {
		// dcl → type ID
		int i = (int) table.symTable.size();
		std::string id;
		std::istringstream(node->children[1]->seq) >> id >> id;
		table.symTable.emplace(id, VarData((-4 * i), node->children[1]->type));
	}

	/************************************/
	/** code generation helper-methods **/
	/************************************/
	/** CONVENTIONS FOR REGISTERS **/
	//
	//  $0 - 0 (CONST)
	//  $1 - first param of wain / param for print
	//  $2 - second param of wain
	//  $3 - return value and intermediate result (MUTABLE)
	//  $4 - 4 (CONST)
	//  $5 - previous intermediate result or print address (MUTABLE)
	//  $6 - scratch register (MUTABLE)
	//  $7 - scratch register (MUTABLE)
	//    ...
	// $11 - 1 (CONST)
	//    ...
	// $29 - frame pointer, fp (SPECIAL)
	// $30 - stack pointer, sp (SPECIAL, initially 0x01000000)
	// $31 - return addr,   ra (SPECIAL, initially 0x8123456c)

	void push(std::ostream &out, int r) {
		out << "\t\tsw $" << r << ", -4($30)" << std::endl;
		out << "\t\tsub $30, $30, $4" << std::endl;
	}

	void pop(std::ostream &out, int r) {
		out << "\t\tadd $30, $30, $4" << std::endl;
		out << "\t\tlw $" << r << ", -4($30)" << std::endl;
	}

	void generate_prog_level(std::ostream &out, Node *node) {
		// start → BOF procedures EOF
		if (node->kind == "start") {
			out << "\t\t.import print" << std::endl;
			out << "\t\t.import init" << std::endl;
			out << "\t\t.import new" << std::endl;
			out << "\t\t.import delete" << std::endl;
			out << "\t\tlis $4"  << std::endl;
			out << "\t\t.word 4" << std::endl;
			out << "\t\tlis $11" << std::endl;
			out << "\t\t.word 1" << std::endl;
			out << "\t\tbeq $0, $0, Fwain" << std::endl;
			generate_prog_level(out, node->children[1]);

		// procedures → main
		// procedures → procedure procedures
		} else {
			generate_proc(out, node->children[0]);
			if (node->children.size() > 1)
				generate_prog_level(out, node->children[1]);
		}
	}

	void generate_proc(std::ostream &out, Node *node) {
		// main → INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
		// procedure → INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE

		/* also use format from context analysis assignment */
		bool isMain = (node->kind == "main");
		int i = (isMain) ? 8 : 6;
		std::string procID;
		std::istringstream(node->children[1]->seq) >> procID >> procID;
		ProcData &table = ptable[procID];




		/* procedure prologue  */
		/* if main function, then store the parameters directly from registers */
		/* otherwise, no code for param - only args require code, will be supplied from caller */
		out << std::endl << std::endl << std::endl << "F" << procID << ":" << std::endl;
		if (isMain) {
			push(out, 31);
			out << "\t\tsub $29, $30, $4" << std::endl;
			out << "\t\tsw $1, 0($29)" << std::endl;
			out << "\t\tsw $2, -4($29)" << std::endl;
		}

		/* update sp to point after the fully initialized stack frame */
		/* must consider space for args AND local variables */
		int offset = ((int) table.symTable.size()) * 4;
		if (offset == 4) {
			out << "\t\tsub $30, $30, $4" << std::endl;

		} else if (offset > 0) {
			out << "\t\tlis $3" << std::endl;
			out << "\t\t.word " << offset << std::endl;
			out << "\t\tsub $30, $30, $3" << std::endl;
		}

		/* initialize the heap allocator */
		if (isMain) {
			if (node->children[3]->children[1]->type == TYPE_INT) {
				out << "\t\tadd $2, $0, $0" << std::endl;
			}
			out << "\t\tlis $5" << std::endl;
			out << "\t\t.word init" << std::endl;
			out << "\t\tjalr $5" << std::endl;
		}




		/* procedure body */
		out << std::endl << std::endl;
		generate_dcls(out, node->children[i], table);
		generate_stmts(out, node->children[i+1], table);
		int r = generate_expr(out, node->children[i+3], table);
		if (r != 3)
			out << "\t\tadd $3, $" << r << ", $0" << std::endl;




		/* procedure epilogue */
		out << std::endl << std::endl;
		out << "\t\tadd $30, $29, $4" << std::endl;
		if (isMain) {
			out << "\t\tlw $1, 0($29)" << std::endl;
			out << "\t\tlw $2, -4($29)" << std::endl;
			pop(out, 31);
			out << "\t\tadd $29, $30, $0" << std::endl;
		}
		out << "\t\tjr $31" << std::endl;
	}

	void generate_dcls(std::ostream &out, Node *node, ProcData &table) {
		// dcls → ε
		// dcls → dcls dcl BECOMES NUM SEMI
		// dcls → dcls dcl BECOMES NULL SEMI
		if (node->children.size() > 0) {
			generate_dcls(out, node->children[0], table);
			generate_dcl(out, node->children[1], table, node->children[3]);
		}
	}

	void generate_dcl(std::ostream &out, Node *node, ProcData &table, Node *valNode) {
		// type → INT
		// type → INT STAR
		// dcl → type ID
		std::string id;
		std::istringstream (node->children[1]->seq) >> id >> id;
		int r = generate_token(out, valNode, table);
		out << "\t\tsw $" << r << ", " << table[id].loc << "($29)" << std::endl;
	}

	void generate_stmts(std::ostream &out, Node *node, ProcData &table) {
		// statements → ε
		// statements → statements statement
		if (node->children.size() > 0) {
			generate_stmts(out, node->children[0], table);
			generate_stmt(out, node->children[1], table);
		}
	}

	void generate_stmt(std::ostream &out, Node *node, ProcData &table) {
		/* produce a comment on the type of statement beforehand */
		out << std::endl << "\t\t;; " << node->seq << std::endl;

		// statement → PRINTLN LPAREN expr RPAREN SEMI
		if (node->children[0]->kind == "PRINTLN") {
			int r = generate_expr(out, node->children[2], table);
			out << "\t\tadd $1, $" << r << ", $0" << std::endl;
			push(out, 31);
			out << "\t\tlis $5" << std::endl;
			out << "\t\t.word print" << std::endl;
			out << "\t\tjalr $5" << std::endl;
			pop(out, 31);

		// statement → IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE
		} else if (node->children[0]->kind == "IF") {
			static int ifC = 0;
			std::string LABEL = table.id + std::to_string(ifC++) + "IFELSE";

			generate_test(out, node->children[2], table);
			out << "\t\tbeq $3, $0, " << LABEL << "FALSE" << std::endl;

			generate_stmts(out, node->children[5], table);
			out << "\t\tbeq $0, $0, " << LABEL << "TRUE" << std::endl;

			out << LABEL << "FALSE:" << std::endl;
			generate_stmts(out, node->children[9], table);
			out << LABEL << "TRUE:" << std::endl;

		// statement → WHILE LPAREN test RPAREN LBRACE statements RBRACE
		} else if (node->children[0]->kind == "WHILE") {
			static int whileC = 0;
			std::string LABEL = table.id + std::to_string(whileC++) + "WHILE";

			out << LABEL << "BODY:" << std::endl;
			generate_test(out, node->children[2], table);
			out << "\t\tbeq $3, $0, " << LABEL << "END" << std::endl;

			generate_stmts(out, node->children[5], table);
			out << "\t\tbeq $0, $0, " << LABEL << "BODY" << std::endl;
			out << LABEL << "END:" << std::endl;

		// statement → DELETE LBRACK RBRACK expr SEMI
		} else if (node->children[0]->kind == "DELETE") {
			static int deleteC = 0;
			std::string LABEL = table.id + std::to_string(deleteC++) + "DELETE";
			int r = generate_expr(out, node->children[3], table);

			out << "\t\tbeq $" << r << ", $11, " << LABEL << std::endl;
			out << "\t\tadd $1, $" << r << ", $0" << std::endl;

			push(out, 31);
			out << "\t\tlis $5" << std::endl;
			out << "\t\t.word delete" << std::endl;
			out << "\t\tjalr $5" << std::endl;
			pop(out, 31);
			out << LABEL << ":" << std::endl;

		// statement → lvalue BECOMES expr SEMI
		} else {
			// sub case: lvalue → LPAREN lvalue RPAREN
			int r = generate_expr(out, node->children[2], table);
			Node *lvalueNode = node->children[0];
			while (lvalueNode->children.size() > 2)
				lvalueNode = lvalueNode->children[1];

			// sub case: lvalue → ID
			if (lvalueNode->children.size() == 1) {
				std::string id;
				std::istringstream(lvalueNode->children[0]->seq) >> id >> id;
				int offset = table[id].loc;
				out << "\t\tsw $" << r << ", " << offset << "($29)" << std::endl;

			// sub case: lvalue → STAR factor
			} else {
				push(out, r);
				r = generate_factor(out, lvalueNode->children[1], table);
				pop(out, 5);
				out << "\t\tsw $5, 0($" << r << ")" << std::endl;
			}
		}
	}

	/** always return test result in $3 **/
	void generate_test(std::ostream &out, Node *node, ProcData &table) {
		int r;
		std::string &kind = node->children[1]->kind;
		std::string op = (node->children[0]->type == TYPE_INT_PTR) ? "sltu" : "slt";

		r = generate_expr(out, node->children[0], table);
		push(out, r);
		r = generate_expr(out, node->children[2], table);
		pop(out, 5);

		// test → expr LT expr
		// test → expr GE expr
		if (kind == "LT" || kind == "GE") {
			out << "\t\t" << op << " $3, $5, $" << r << std::endl;

		// test → expr GT expr
		// test → expr LE expr
		} else if (kind == "GT" || kind == "LE") {
			out << "\t\t" << op << " $3, $" << r << ", $5" << std::endl;

		// test → expr NE expr
		// test → expr EQ expr
		} else {
			out << "\t\t" << op << " $6, $5, $" << r << std::endl;
			out << "\t\t" << op << " $7, $" << r << ", $5" << std::endl;
			out << "\t\tadd $3, $6, $7" << std::endl;
		}

		if (kind == "GE" || kind == "LE" || kind == "EQ")
			out << "\t\tsub $3, $11, $3" << std::endl;
	}

	/** For all expression generation methods, return value is the register **/
	/** number containing the value ($3 by default, or others when optimizing) **/
	int generate_expr(std::ostream &out, Node *node, ProcData &table) {
		// expr → term
		if (node->children.size() == 1)
			return generate_term(out, node->children[0], table);

		/* optimizing: constant folding (compile-time computation) */
		// expr → expr PLUS term
		// expr → expr MINUS term
		Node *left = node->children[0]->children[0]->children[0]->children[0];	// optimize if NUM and...  (guaranteed existence)
		Node *right = node->children[2]->children[0]->children[0];				// optimize if NUM as well (guaranteed existence)
		if (left->kind == "NUM" && right->kind == "NUM") {
			int x, y;
			std::string str;
			std::istringstream(left->seq) >> str >> x;
			std::istringstream(right->seq) >> str >> y;

			x = (node->children[1]->kind == "PLUS") ? x + y : x - y;
			out << "\t\tlis $3" << std::endl;
			out << "\t\t.word " << x << std::endl;
			return 3;

		// expr → expr PLUS term
		// expr → expr MINUS term
		} else {
			int q = 5;	// register holding left hand side calculation prior to performing operation
			int r;		// register holding right hand side calculation prior to performing operation
			bool isPlus = (node->children[1]->kind == "PLUS");		// otherwise MINUS
			bool ptrArith = (isPlus)
						  ? (node->children[0]->type != node->children[2]->type)
						  : (node->children[0]->type == TYPE_INT_PTR);
			std::string op = (isPlus) ? "add" : "sub";

			// sub case: typeof(expr, op, term) = (int, ±, int)
			r = generate_expr(out, node->children[0], table);
			if (ptrArith && node->children[0]->type == TYPE_INT) {
				// sub case: typeof(expr, op, term) = (int, +, int*)
				out << "\t\tmult $" << r << ", $4" << std::endl;
				out << "\t\tmflo $" << (r = 3) << std::endl;
			}

			/* STACK REGISTER OPTIMIZATION */
			// std::cerr << "\t\t\033[0;31m (" << MIN_REG << ", " << stackReg << ", " << MAX_REG << ") \033[0m" << std::endl;
			if (stackReg <= MAX_REG) {
				/* use reg now to access first param instead of popping to $5 */
				out << "\t\tadd $" << stackReg << ", $" << r << ", $0" << std::endl;
				q = stackReg++;
			} else {
				/* retain old system when stack registers are exhausted */
				push(out, r);
				++stacked;
			}
			// std::cerr << "\t\t\033[0;31m (" << MIN_REG << ", " << stackReg << ", " << MAX_REG << ") \033[0m" << std::endl;
			/* STACK REGISTER OPTIMIZATION */

			r = generate_term(out, node->children[2], table);
			if (ptrArith && node->children[2]->type == TYPE_INT) {
				// sub case: typeof(expr, op, term) = (int*, ±, int)
				out << "\t\tmult $" << r << ", $4" << std::endl;
				out << "\t\tmflo $" << (r = 3) << std::endl;
			}

			/* STACK REGISTER OPTIMIZATION */
			if (q == 5) {
				/* was no more stack registers, retrieve from actual stack */
				pop(out, 5);
				--stacked;
			}
			/* STACK REGISTER OPTIMIZATION */

			out << "\t\t" << op << " $3, $" << q << ", $" << r << std::endl;
			if (ptrArith && node->children[0]->type == node->children[2]->type) {
				// sub case: typeof(expr, op, term) = (int*, -, int*)
				out << "\t\tdiv $3, $4" << std::endl;
				out << "\t\tmflo $3" << std::endl;
			}

			/* STACK REGISTER OPTIMIZATION */
			/* free stack register if used */
			if (q != 5) --stackReg;
			/* STACK REGISTER OPTIMIZATION */
			return 3;
		}
	}

	int generate_term(std::ostream &out, Node *node, ProcData &table) {
		// term → factor
		if (node->children.size() == 1)
			return generate_factor(out, node->children[0], table);

		/* optimizing: constant folding (compile-time computation) */
		// term → term STAR factor
		// term → term SLASH factor
		// term → term PCT factor
		Node *left = node->children[0]->children[0]->children[0];	// optimize if NUM and...  (guaranteed existence)
		Node *right = node->children[2]->children[0];				// optimize if NUM as well (guaranteed existence)
		if (left->kind == "NUM" && right->kind == "NUM") {
			int x, y;
			std::string str;
			std::istringstream(left->seq) >> str >> x;
			std::istringstream(right->seq) >> str >> y;

			x = (node->children[1]->kind == "STAR") ? x * y
			  : (node->children[1]->kind == "SLASH") ? x / y : x % y;
			out << "\t\tlis $3" << std::endl;
			out << "\t\t.word " << x << std::endl;
			return 3;

		// term → term STAR factor
		// term → term SLASH factor
		// term → term PCT factor
		} else {
			int q = 5;
			int r;
			std::string op = (node->children[1]->kind == "STAR") ? "mult" : "div";
			std::string mf = (node->children[1]->kind == "PCT") ? "mfhi" : "mflo";

			r = generate_term(out, node->children[0], table);
			/* STACK REGISTER OPTIMIZATION */
			// std::cerr << "\t\t\033[0;31m (" << MIN_REG << ", " << stackReg << ", " << MAX_REG << ") \033[0m" << std::endl;
			if (stackReg <= MAX_REG) {
				/* use reg now to access first param instead of popping to $5 */
				out << "\t\tadd $" << stackReg << ", $" << r << ", $0" << std::endl;
				q = stackReg++;
			} else {
				/* retain old system when stack registers are exhausted */
				push(out, r);
				++stacked;
			}
			// std::cerr << "\t\t\033[0;31m (" << MIN_REG << ", " << stackReg << ", " << MAX_REG << ") \033[0m" << std::endl;
			/* STACK REGISTER OPTIMIZATION */

			r = generate_factor(out, node->children[2], table);
			/* STACK REGISTER OPTIMIZATION */
			if (q == 5) {
				/* was no more stack registers, retrieve from actual stack */
				pop(out, 5);
				--stacked;
			}
			/* STACK REGISTER OPTIMIZATION */

			out << "\t\t" << op << " $" << q << ", $" << r << std::endl;
			out << "\t\t" << mf << " $3" << std::endl;

			/* STACK REGISTER OPTIMIZATION */
			/* free stack register if used */
			if (q != 5) --stackReg;
			/* STACK REGISTER OPTIMIZATION */
			return 3;
		}
	}

	int generate_factor(std::ostream &out, Node *node, ProcData &table) {
		// factor → NUM
		// factor → ID
		// factor → NULL
		if (node->children.size() == 1) {
			return generate_token(out, node->children[0], table);
		
		// factor → LPAREN expr RPAREN
		} else if (node->children[0]->kind == "LPAREN") {
			return generate_expr(out, node->children[1], table);

		// factor → AMP lvalue
		} else if (node->children[0]->kind == "AMP") {
			Node *lvalueNode = node->children[1];

			// sub case: lvalue → LPAREN lvalue RPAREN
			/* dispel all layers of parentheses */
			while (lvalueNode->children.size() > 2)
				lvalueNode = lvalueNode->children[1];

			// sub case: lvalue → STAR factor
			if (lvalueNode->children[0]->kind == "STAR")
				return generate_factor(out, lvalueNode->children[1], table);

			// sub case: lvalue → ID
			std::string id;
			std::istringstream(lvalueNode->children[0]->seq) >> id >> id;
			int offset = table[id].loc;
			if (offset == 0) return 29;

			if (offset == -4) {
				out << "\t\tsub $3, $29, $4" << std::endl;
			} else {
				out << "\t\tlis $3" << std::endl;
				out << "\t\t.word " << offset << std::endl;
				out << "\t\tadd $3, $29, $3" << std::endl;
			}

		// factor → STAR factor
		} else if (node->children[0]->kind == "STAR") {
			int r = generate_factor(out, node->children[1], table);
			out << "\t\tlw $3, 0($" << r << ")" << std::endl;

		// factor → NEW INT LBRACK expr RBRACK
		} else if (node->children[0]->kind == "NEW") {
			int r = generate_expr(out, node->children[3], table);
			out << "\t\tadd $1, $" << r << ", $0" << std::endl;

			push(out, 31);
			out << "\t\tlis $5" << std::endl;
			out << "\t\t.word new" << std::endl;
			out << "\t\tjalr $5" << std::endl;
			pop(out, 31);

			out << "\t\tbne $3, $0, 1" << std::endl;
			out << "\t\tadd $3, $11, $0" << std::endl;

		// factor → ID LPAREN RPAREN
		// factor → ID LPAREN arglist RPAREN
		} else {
			int pushC = 3;			// 1 + number of registers to preserve
			std::string procID;
			std::istringstream(node->children[0]->seq) >> procID >> procID;

			/* save fp, ra, and any stack registers using mass push */
				// push(out, 29);
				// push(out, 31);
			out << "\t\tsw $29, -4($30)" << std::endl;
			out << "\t\tsw $31, -8($30)" << std::endl;
			for (int sr = MIN_REG; sr < stackReg; ++sr, ++pushC)
				out << "\t\tsw $" << sr << ", -" << (4 * pushC) << "($30)" << std::endl;
			out << "\t\tlis $5" << std::endl;
			out << "\t\t.word " << (4 * (pushC-1)) << std::endl;
			out << "\t\tsub $30, $30, $5" << std::endl;

			/* compute and store each arg, then set new fp */
			if (node->children[2]->kind == "arglist") {
				int argc = generate_args(out, node->children[2], table);
				if (argc == 1) {
					out << "\t\tadd $30, $30, $4" << std::endl;
				} else {
					out << "\t\tlis $5" << std::endl;
					out << "\t\t.word " << (4 * argc) << std::endl;
					out << "\t\tadd $30, $30, $5" << std::endl;
				}
			}
			out << "\t\tsub $29, $30, $4" << std::endl;

			/* call procedure */
			out << "\t\tlis $5" << std::endl;
			out << "\t\t.word F" << procID << std::endl;
			out << "\t\tjalr $5" << std::endl;

			/* reset the stack */
				// pop(out, 31);
				// pop(out, 29);
			out << "\t\tlis $5" << std::endl;
			out << "\t\t.word " << (4 * (pushC-1)) << std::endl;
			out << "\t\tadd $30, $30, $5" << std::endl;
			out << "\t\tlw $29, -4($30)" << std::endl;
			out << "\t\tlw $31, -8($30)" << std::endl;
			pushC = 3;
			for (int sr = MIN_REG; sr < stackReg; ++sr, ++pushC)
				out << "\t\tlw $" << sr << ", -" << (4 * pushC) << "($30)" << std::endl;
		}
		return 3;
	}

	/** return arg count - push the expression results onto frame in proper order **/
	int generate_args(std::ostream &out, Node *node, ProcData &table, int i = 1) {
		// arglist → expr
		// arglist → expr COMMA arglist
		int r = generate_expr(out, node->children[0], table);
		push(out, r);
		if (node->children.size() == 1) return i;
		return generate_args(out, node->children[2], table, i+1);
	}

	int generate_token(std::ostream &out, Node *node, ProcData &table) {
		// NUM || NULL || ID
		std::string str;
		std::istringstream(node->seq) >> str >> str;

		if (node->kind == "NULL") {
			return 11;

		} else if (node->kind == "ID")  {
			out << "\t\tlw $3, " << table[str].loc << "($29)" << std::endl;
			return 3;

		} else {
			int val = std::stoi(str);
			if (val == 1) return 11;
			if (val == 0 || val == 4) return val;
			out << "\t\tlis $3" << std::endl;
			out << "\t\t.word " << val << std::endl;
			return 3;
		}
	}

	/*******************************************/
	/** end of code generation helper-methods **/
	/*******************************************/

	CFG &cfg;
	Node *root;
	std::map<std::string,ProcData> ptable;
	int stackReg = MIN_REG;					// "stack register" - use as efficient quick global stack
	int stacked = 0;						// if we run out of stack registers, resort to using stack as usual
  public:
	WLP4ParseTree(CFG &cfg)
		: cfg(cfg), root(nullptr), ptable(), stackReg(MIN_REG), stacked(0) {}

	WLP4ParseTree(const WLP4ParseTree &tree)
		: cfg(tree.cfg), root(tree.root), ptable(tree.ptable), stackReg(tree.stackReg), stacked(tree.stacked) {}

	~WLP4ParseTree() { delete root; }

	WLP4ParseTree &operator=(const WLP4ParseTree &tree) {
		// cfg already exists
		// shallow copy root, since if copied, copy will be deleted (intended)
		// ptable already exists
		root = tree.root;
		return *this;
	}

	/** Main code generator **/
	/** Output directly to stream **/
	std::ostream &generate(std::ostream &out = std::cout) {
		generate_prog_level(out, root);
		return out;
	}

	friend std::istream &operator>>(std::istream &in, WLP4ParseTree &tree);
};

std::istream &operator>>(std::istream &in, WLP4ParseTree &tree) {
	if (tree.root != nullptr) delete tree.root;
	tree.root = tree.readTree(in);
	tree.initptable(tree.root);
	return in;
}




int main() {
	std::string str, word;
	CFG wlp4cfg;
	WLP4ParseTree tree(wlp4cfg);

	// initialize the wlp4 CFG
	std::istringstream in(WLP4_CFG);
	getline(in, str);	// skip ".CFG" line

	while (getline(in, str)) {
		std::string nt;
		std::vector<std::string> rule;
		std::istringstream iss(str);

		iss >> nt;
		while (iss >> word) {
			if (word != DIR_EMPTY) rule.push_back(word);
		}
		wlp4cfg.addProd(nt, rule);
	}

	// read in the parse tree, annotate, then output
	std::cin >> tree;
	tree.generate(std::cout);
}
