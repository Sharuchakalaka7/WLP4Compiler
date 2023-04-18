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

		int getAllCount() { return rule.size(); }			// total symbol count in rule

		int getNTCount() { return ntCount; }				// number of non-terminals in rule

		int getTCount() { return rule.size() - ntCount; }	// number of terminals in rule

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

	/** Internal error handling state **/
	class TypeError {
		std::string msg;
	  public:
		TypeError(std::string msg) : msg(msg) {}
		std::string what() {
			return "ERROR: " + msg;
		}
	};

	/** Internal data structure for individual procedures **/
	struct ProcData {
		std::string id;
		std::vector<std::string> signature;
		std::map<std::string,std::string> symTable;
		ProcData() : id(""), signature(), symTable() {}
		ProcData(std::string &id) : id(id), signature(), symTable() {}
		std::string &operator[](std::string &varID) { return symTable[varID]; }
		int count(std::string &varID) { return symTable.count(varID); }
	};

	/** recursive IO functions for the trees **/
	Node *readTree(std::istream &in) {
		// altered version of depth first search/ pre-order traversal
		std::string str;
		std::string word;

		if (getline(in, str)) {
			std::istringstream iss(str);
			iss >> word;
			Node *node = new Node(word, str);		// base case

			// recursive case
			// if first thing is terminal, then no children (must be a leaf of parse tree)
			// otherwise, first is non-terminal means child nodes for each proceding rule symbol
			if (cfg.isNonTerminal(word)) {
				while (iss >> word && word != DIR_EMPTY) {
					Node *child = readTree(in);
					if (child != nullptr) node->children.push_back(child);
				}
			}
			return node;
		}
		return nullptr;
	}

	void printTree(std::ostream &out, Node *node) {
		out << node->seq << ((node->type == TYPE_NONE) ? "" : " : " + node->type) << std::endl;
		for (Node *c : node->children) printTree(out, c);
	}

	/*****************************/
	/** annotate helper-methods **/
	/*****************************/

	void annotate_prog_level(Node *node) {
		// start → BOF procedures EOF
		if (node->kind == "start") {
			annotate_prog_level(node->children[1]);

		// procedures → main
		// procedures → procedure procedures
		} else if (node->kind == "procedures") {
			annotate_proc(node->children[0]);
			if (node->children.size() > 1)
				annotate_prog_level(node->children[1]);

		} else {
			throw TypeError("(FATAL) Not valid production rule - " + node->kind);
		}
	}

	void annotate_proc(Node *node) {
		// procedure → INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
		// main → INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE

		int i;
		bool isMain = (node->kind == "main");
		std::string procID;
		std::istringstream iss(node->children[1]->seq);

		/* for any proc, including main, need to check with the proc table */
		iss >> procID >> procID;
		if (ptable.count(procID) > 0)
			throw TypeError("Procedure " + procID + "is already declared.");
		ProcData &table = ptable[procID] = ProcData(procID);

		/* difference at the parameter level, but same elsewhere */
		if (isMain) {
			annotate_dcl(node->children[3], table);
			if (annotate_dcl(node->children[5], table) != TYPE_INT)
				throw TypeError("The second parameter of wain is not int type.");
			i = 8;

		} else {
			annotate_params(node->children[3], table);
			i = 6;
		}

		annotate_dcls(node->children[i], table);
		annotate_stmts(node->children[i+1], table);

		if (annotate_expr(node->children[i+3], table) != TYPE_INT)
			throw TypeError("The return expression of [" + procID + "] is not int type.");
	}

	void annotate_params(Node *node, ProcData &table) {
		// params → ε
		// params → paramlist
		if (node->kind == "params") {
			if (node->children.size() > 0)
				annotate_params(node->children[0], table);

		// paramlist → dcl
		// paramlist → dcl COMMA paramlist
		} else if (node->kind == "paramlist") {
			table.signature.push_back(annotate_dcl(node->children[0], table));
			if (node->children.size() > 1)
				annotate_params(node->children[2], table);

		} else {
			throw TypeError("(FATAL) Not valid production rule - " + node->kind);
		}
	}

	void annotate_dcls(Node *node, ProcData &table) {
		// dcls → ε
		// dcls → dcls dcl BECOMES NUM SEMI
		// dcls → dcls dcl BECOMES NULL SEMI
		if (node->children.size() > 0) {
			annotate_dcl(node->children[1], table, node->children[3]);
			annotate_dcls(node->children[0], table);
		}
	}

	std::string &annotate_dcl(Node *node, ProcData &table, Node *rvalueNode = nullptr) {
		// dcl → type ID
		Node *typeNode = node->children[0];
		Node *idNode = node->children[1];

		// first get id information and check its existence
		std::string id;
		std::istringstream iss(idNode->seq);

		iss >> id >> id;
		if (table.count(id) != 0)
			throw TypeError("Variable " + id + " is already declared.");

		// find type information
			// type → INT
			// type → INT STAR
		std::string type = (typeNode->children.size() == 1) ? TYPE_INT : TYPE_INT_PTR;

		// handle rvalue in case definition also included
		if (rvalueNode != nullptr && annotate_token(rvalueNode, table) != type)
			throw TypeError("Expected type " + type + " when initializing " + id + " in [" + table.id + "].");

		// set type and return 
		return idNode->type = table[id] = type;
	}

	void annotate_stmts(Node *node, ProcData &table) {
		// statements → ε
		// statements → statements statement
		if (node->children.size() > 0) {
			annotate_stmts(node->children[0], table);
			annotate_stmt(node->children[1], table);
		}
	}

	void annotate_stmt(Node *node, ProcData &table) {
		// statement → lvalue BECOMES expr SEMI
		if (node->children[0]->kind == "lvalue") {
			std::string &lvalueType = annotate_lvalue(node->children[0], table);
			if (annotate_expr(node->children[2], table) != lvalueType)
				throw TypeError("Expected same type in assignment variable and new value.");

		// statement → IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE
		} else if (node->children[0]->kind == "IF") {
			annotate_test(node->children[2], table);
			annotate_stmts(node->children[5], table);
			annotate_stmts(node->children[9], table);

		// statement → WHILE LPAREN test RPAREN LBRACE statements RBRACE
		} else if (node->children[0]->kind == "WHILE") {
			annotate_test(node->children[2], table);
			annotate_stmts(node->children[5], table);

		// statement → PRINTLN LPAREN expr RPAREN SEMI
		} else if (node->children[0]->kind == "PRINTLN") {
			if (annotate_expr(node->children[2], table) != TYPE_INT)
				throw TypeError("Expected type " + TYPE_INT + " in PRINTLN.");

		// statement → DELETE LBRACK RBRACK expr SEMI
		} else if (node->children[0]->kind == "DELETE") {
			if (annotate_expr(node->children[3], table) != TYPE_INT_PTR)
				throw TypeError("Expected type " + TYPE_INT_PTR + " in DELETE.");

		} else {
			throw TypeError("(FATAL) Not valid production rule - " + node->kind);
		}
	}

	void annotate_test(Node *node, ProcData &table) {
		// test → expr EQ expr
		// test → expr NE expr
		// test → expr LT expr
		// test → expr LE expr
		// test → expr GE expr
		// test → expr GT expr
		if (annotate_expr(node->children[0], table) != annotate_expr(node->children[2], table))
			throw TypeError("Type mismatch in boolean expression.");
	}
	
	std::string &annotate_expr(Node *node, ProcData &table) {
		// expr → term
		std::string &termType = annotate_term(node->children.back(), table);
		if (node->children.size() == 1) return node->type = termType;

		// expr → expr PLUS term
		// expr → expr MINUS term
		std::string &exprType = annotate_expr(node->children[0], table);
		if (termType == TYPE_INT) {
			node->type = exprType;

		} else if (node->children[1]->kind == "PLUS") {
			if (exprType != TYPE_INT)
				throw TypeError("Expected expression {" + TYPE_INT + " + " + TYPE_INT_PTR + "}, given {" + exprType + " + " + termType + "}.");
			node->type = TYPE_INT_PTR;

		} else {
			if (exprType != TYPE_INT_PTR)
				throw TypeError("Expected expression {" + TYPE_INT_PTR + " - " + TYPE_INT_PTR + "}, given {" + exprType + " - " + termType + "}.");
			node->type = TYPE_INT;
		}
		return node->type;
	}

	std::string &annotate_term(Node *node, ProcData &table) {
		// term → factor
		// term → term STAR factor
		// term → term SLASH factor
		// term → term PCT factor
		node->type = annotate_factor(node->children.back(), table);
		if (node->children.size() > 1 && (node->type != TYPE_INT || annotate_term(node->children[0], table) != TYPE_INT))
			throw TypeError("Expected multiple combined factors to all have type int.");
		return node->type;
	}

	std::string &annotate_factor(Node *node, ProcData &table) {
		// factor → NUM  
		// factor → NULL
		// factor → ID
		if (node->children.size() == 1) {
			node->type = annotate_token(node->children[0], table);

		// factor → ID LPAREN RPAREN
		// factor → ID LPAREN arglist RPAREN
		} else if (node->children[0]->kind == "ID") {
			// calling function - ensure correct function name
			std::string procID;
			std::istringstream iss(node->children[0]->seq);

			iss >> procID >> procID;
			if (procID == "wain")
				throw TypeError("Cannot call main procedure [wain].");
			if (procID == table.id && table.count(procID) != 0)
				throw TypeError("Cannot call recurse procedure [" + procID + "] since declared as a local variable already.");
			if (ptable.count(procID) == 0)
				throw TypeError("Procedure [" + procID + "] called before declaration.");

			// ensure argument sequence matches by length and exact ordered types
			if (node->children[2]->kind == "arglist")
				annotate_args(node->children[2], table, ptable[procID]);
			else if (ptable[procID].signature.size() != 0)
				throw TypeError("Arity mismatch - expected no args in [" + procID + "].");

			node->type = TYPE_INT;

		// factor → LPAREN expr RPAREN
		} else if (node->children.size() == 3) {
			node->type = annotate_expr(node->children[1], table);

		// factor → NEW INT LBRACK expr RBRACK
		} else if (node->children.size() == 5) {
			if (annotate_expr(node->children[3], table) != TYPE_INT)
				throw TypeError("Expected INT in array declaration size, given - " + TYPE_INT_PTR + ".");
			node->type = TYPE_INT_PTR;

		// factor → AMP lvalue
		} else if (node->children[0]->kind == "AMP") {
			if (annotate_lvalue(node->children[1], table) != TYPE_INT)
				throw TypeError("Expected int when referencing, given - " + TYPE_INT_PTR + ".");
			node->type = TYPE_INT_PTR;

		// factor → STAR factor
		} else if (node->children[0]->kind == "STAR") {
			if (annotate_factor(node->children[1], table) != TYPE_INT_PTR)
				throw TypeError("Expected int* when dereferencing, given - " + TYPE_INT + ".");
			node->type = TYPE_INT;

		} else {
			throw TypeError("(FATAL) Not valid production rule - " + node->kind);
		}
		return node->type;
	}

	void annotate_args(Node *node, ProcData &table, ProcData &callTable, unsigned int idx = 0) {
		// arglist → expr
		// arglist → expr COMMA arglist
		if (callTable.signature.size() == idx)
			throw TypeError("Too many args for [" + callTable.id + "].");
		if (node->children.size() == 1 && idx != callTable.signature.size()-1)
			throw TypeError("Too few args for [" + callTable.id + "].");

		std::string &argType = annotate_expr(node->children[0], table);
		if (argType != callTable.signature[idx])
			throw TypeError("Arity type mismatch when calling [" + callTable.id + "].");

		if (node->children.size() > 1)
			annotate_args(node->children[2], table, callTable, idx+1);
	}

	std::string &annotate_lvalue(Node *node, ProcData &table) {
		switch (node->children.size()) {
			// lvalue → ID
			case 1:
				node->type = annotate_token(node->children[0], table);
				break;

			// lvalue → STAR factor
			case 2:
				if (annotate_factor(node->children[1], table) != TYPE_INT_PTR)
					throw TypeError("Expected int* when dereferencing, given - " + TYPE_INT + ".");
				node->type = TYPE_INT;
				break;

			// lvalue → LPAREN lvalue RPAREN
			case 3:
				node->type = annotate_lvalue(node->children[1], table);
				break;

			default:
				throw TypeError("(FATAL) Not valid production rule - " + node->kind);
		}
		return node->type;
	}

	std::string &annotate_token(Node *node, ProcData &table) {
		// terminal cases - NUM, NULL, ID
		if (node->kind == "NUM") {
			node->type = TYPE_INT;

		} else if (node->kind == "NULL") {
			node->type = TYPE_INT_PTR;

		} else if (node->kind == "ID") {
			std::string id;
			std::istringstream iss(node->seq);

			iss >> id >> id;
			if (table.count(id) == 0)
				throw TypeError("Undeclared variable " + id + ".");
			node->type = table[id];

		} else {
			throw TypeError("(FATAL) Not valid expression token kind - " + node->kind);
		}
		return node->type;
	}

	/************************************/
	/** end of annotate helper-methods **/
	/************************************/

	CFG &cfg;
	Node *root;
	std::map<std::string,ProcData> ptable;		// full procedures table
  public:
	WLP4ParseTree(CFG &cfg) : cfg(cfg), root(nullptr), ptable() {}
	~WLP4ParseTree() { delete root; }
	friend std::istream &operator>>(std::istream &in, WLP4ParseTree &tree);
	friend std::ostream &operator<<(std::ostream &out, WLP4ParseTree &tree);

	/** Perform semantic error checking and assign types **/
	/** errors checked in leveled case-wise fashion, then returns status **/
	bool annotate(std::ostream &err = std::cerr) {
		try {
			annotate_prog_level(root);
			return true;
		} catch (TypeError &te) {
			err << te.what() << std::endl;
			return false;
		}
	}
};

std::istream &operator>>(std::istream &in, WLP4ParseTree &tree) {
	if (tree.root != nullptr) delete tree.root;
	tree.root = tree.readTree(in);
	return in;
}

std::ostream &operator<<(std::ostream &out, WLP4ParseTree &tree) {
	tree.printTree(out, tree.root);
	return out;
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
	if (tree.annotate()) {
		std::cout << tree;
	}
}
