#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include "wlp4data.h"

std::string DIR_CFG = ".CFG";
std::string DIR_DERIVATIONS = ".DERIVATION";
std::string DIR_INPUT = ".INPUT";
std::string DIR_ACTIONS = ".ACTIONS";
std::string DIR_TRANSITIONS = ".TRANSITIONS";
std::string DIR_REDUCTIONS = ".REDUCTIONS";
std::string DIR_END = ".END";
std::string DIR_EMPTY = ".EMPTY";
std::string DIR_ACCEPT = ".ACCEPT";
std::string STR_BOF = "BOF";
std::string STR_EOF = "EOF";




// S := State name type, T:= Transition type
// originally, S := std::string, T := char
template <typename S, typename T>
class DFA {	
	// DFA class refers to the entire state machine (graph) consisting of
	// - (definition of a state(node), as the class "State")
	// - a tagged collection of all the states involved in the DFA, which are all automatically managed (memory and all)
	// - the initial state for the defined state machine
  protected:
	class State {
		// State class refers to each state (node) in the state machine (graph), consisting of
		// - the state's name/identifier
		// - accepting status of the state
		// - all transitions to other states (conditional, unidirectional edges)
		S name;
		bool accepting;
		std::map<T,State*> next;
	  public:
		/** State constructor must know state's name and accepting condition **/
		State(S s, bool b) : name{s}, accepting{b}, next() {}

		/** Add a new such node-to-node edge **/
		void addTransition(T sym, State *nextState) {
			next[sym] = nextState;
		}

		/** Follow an edge to a connecting node and return the node, if possible **/
		State *transition(T sym) {
			return (next.count(sym) > 0) ? next[sym] : nullptr;
		}

		/** Return state's name **/
		S getName() {
			return name;
		}

		/** Return accepting condition **/
		bool isAccepting() {
			return accepting;
		}
	};

	std::map<S, State*> states;
	State *start;
  public:
	/** DFA constructor must contain the start state at least **/
	DFA(S s, bool b) : states(), start{new State(s, b)} {
		states[s] = start;
	}

	/** Delete all memory from states owned by the DFA **/
	~DFA() {
		for (auto &kv : states) {
			delete kv.second;
		}
	}

	/** Create another state in the DFA (initially with no edges) **/
	void addState(S s, bool b) {
		if (states.count(s) == 0)
			states[s] = new State(s, b);
	}

	/** Create a transition (conditional, unidirectional edge) between two states **/
	void addTransition(S from, T sym, S to) {
		if (states.count(from) > 0 && states.count(to) > 0)
			states[from]->addTransition(sym, states[to]);
	}
};




class CFG {
	// Class CFG defines Context Free Grammars as known and used usually. Consists of:
	// - the start symbol non-terminal symbol
	// - the list of production rules, numbered by each natural number
  protected:
	class Production {
		// Class Production defines and stores a production rule and all its data:
		// - the left hand non-terminal for which the rule is defined
		// - the sequence of terminals/non-terminals that define the rule
		std::string nt;
		std::vector<std::string> rule;
	  public:
		Production(std::string nt, std::vector<std::string> &rule) : nt{nt}, rule(rule) {}

		/** Only have simple production rule accessor methods **/
		std::string &getNT() { return nt; }

		int getCount() { return rule.size(); }

		std::vector<std::string> &getRule() { return rule; }
	};

	std::string start;
	std::vector<Production> prods;
  public:
	CFG() : start{""}, prods() {}

	/** Extend production rule accessor methods by pinpointing numbering **/
	std::string &getProdNT(int n) {
		return prods.at(n).getNT();
	}

	int getProdCount(int n) {
		return prods.at(n).getCount();
	}

	std::vector<std::string> &getProdRule(int n) {
		return prods.at(n).getRule();
	}

	/** Add a new production rule **/
	void addProd(std::string &nt, std::vector<std::string> &rule) {
		if (prods.size() == 0) start = nt;
		prods.push_back(Production(nt, rule));
	}
};




class WLP4Parser : public DFA<int,std::string> {
	// Class BUP is a DFA that defines "Bottom Up Parser" instances using a predefined CFG. Contains:
	// - the DFA as defined throughout
	// - the augmented input as a sequence of terminals (include BOF and EOF)
	// - all reduction possibilities for each state in the DFA
  protected:
	struct Token {
		std::string kind;
		std::string lexeme;
		Token(std::string &kind, std::string &lexeme) : kind(kind), lexeme(lexeme) {}
	};

	struct Node {
		const int prod;
		const Token *tokptr;
		std::vector<Node*> children;
		// Specific constructor for the terminals case
		Node(Token *tokptr) : prod(-1), tokptr(tokptr) {}
		// Specific constructor for the non-terminals case
		Node(int prod) : prod(prod), tokptr(nullptr) {}
		// Need to deallocate all children at destruction
		~Node() {
			for (Node *child : children) delete child;
		}
	};

	CFG cfg;
	std::vector<Token> input;
	std::vector<std::map<std::string,int>> reductions;

	/** Parsing actions, including SLR(1) **/

	/** Reduce stage **/
	void reduce(std::vector<Node*> &nodeStack, std::vector<State*> &stateStack, Token &a) {
		std::map<std::string,int> &M = reductions[stateStack.back()->getName()];
		Node *newNT = new Node(M[a.kind]);

		for (int i = 0; i < cfg.getProdCount(M[a.kind]); ++i) {
			newNT->children.insert(newNT->children.begin(), nodeStack.back());
			nodeStack.pop_back();
			stateStack.pop_back();
		}
		nodeStack.push_back(newNT);
		stateStack.push_back((stateStack.empty()) ? start : stateStack.back()->transition(cfg.getProdNT(M[a.kind])));
	}

	/** Shift stage **/
	bool shift(std::vector<Node*> &nodeStack, std::vector<State*> &stateStack, Token &a) {
		nodeStack.push_back(new Node(&a));
		State *nextState = stateStack.back()->transition(a.kind);
		if (nextState == nullptr) return true;
		stateStack.push_back(nextState);
		return false;
	}

	/** Print parse progress **/
	std::ostream &print(std::ostream &out, Node *node) {
		if (node->prod < 0) {
			out << node->tokptr->kind << ' ' << node->tokptr->lexeme;
		} else {
			out << cfg.getProdNT(node->prod);
			if (cfg.getProdCount(node->prod) > 0) {
				for (auto &r : cfg.getProdRule(node->prod)) {
					out << ' ' << r;
				}
			} else {
				out << ' ' << DIR_EMPTY;
			}
		}
		out << std::endl;
		for (Node *child : node->children)
			print(out, child);
		return out;
	}

	/** Deallocate all parse trees **/
	void deleteNodeStack(std::vector<Node*> &nodeStack) {
		while (!nodeStack.empty()) {
			delete nodeStack.back();
			nodeStack.pop_back();
		}
	}

	/** SLR(1) Algorithm **/
	std::ostream &slr1(std::vector<Token> &input, std::ostream &out, std::ostream &err) {
		std::vector<Node*> nodeStack;
		std::vector<State*> stateStack;

		// Initialize Stage
		nodeStack.push_back(new Node(&input[0]));
		stateStack.push_back(start->transition(input[0].kind));

		// Run loop with k as the counter, as used in the error messages
		for (unsigned int k = 1; k < input.size(); ++k) {
			Token &a = input[k];
			while (reductions[stateStack.back()->getName()].count(a.kind) > 0) {
				reduce(nodeStack, stateStack, a);
			}
			if (shift(nodeStack, stateStack, a)) {
				deleteNodeStack(nodeStack);
				return err << "ERROR at " << k << std::endl;
			}
		}

		// Accept Stage
		Token a(DIR_ACCEPT, DIR_ACCEPT);
		reduce(nodeStack, stateStack, a);
		print(out, nodeStack[0]);
		deleteNodeStack(nodeStack);

		return out;
	}

  public:
	WLP4Parser() : DFA(0, true), cfg(), input(), reductions() {
		// To initialize, need to build everything in the original format of the .cfg files
		// except for the .INPUT component
		int n, m;
		std::string str, word;
		std::istringstream in(WLP4_COMBINED);

		// 1) read .CFG file component
		// initialize the internal CFG specifications
		getline(in, str);
		while (getline(in, str) && str != DIR_TRANSITIONS) {
			std::string nt;
			std::vector<std::string> rule;
			std::istringstream iss(str);

			iss >> nt;
			while (iss >> word) {
				if (word != DIR_EMPTY) rule.push_back(word);
			}
			cfg.addProd(nt, rule);
		}

		// 2) read .TRANSITIONS file component
		// initialize full internal DFA base component
		while (getline(in, str) && str != DIR_REDUCTIONS) {
			std::istringstream iss(str);
			iss >> n;		// from state
			iss >> word;	// transition symbol
			iss >> m;		// to state

			if (std::max(n, m) >= (int) states.size()) {
				int s = states.size();
				while (s <= std::max(n, m)) {
					DFA::addState(s++, true);
				}
			}
			addTransition(n, word, m);
		}

		// 3) read .REDUCTIONS file component
		// initialize reductions rules and look-ahead for each state
		for (unsigned int i = 0; i < states.size(); ++i) {
			reductions.emplace_back();
		}
		while (getline(in, str) && str != DIR_END) {
			std::istringstream iss(str);
			iss >> n;		// state number
			iss >> m;		// rule number
			iss >> word;	// lookahead symbol

			reductions[n][word] = m;
		}

		// Only remains to perform modified SLR(1) on raw input tokens as given
	}

	/** Main parse managing method **/
	std::ostream &parse(const std::string &raw, std::ostream &out = std::cout, std::ostream &err = std::cerr) {
		std::string kind, lexeme;
		std::vector<Token> input;
		std::istringstream iss(raw);

		// Augment the input with BOF AND EOF
		input.emplace_back(STR_BOF, STR_BOF);
		while (iss >> kind && iss >> lexeme)
			input.emplace_back(kind, lexeme);
		input.emplace_back(STR_EOF, STR_EOF);
		return slr1(input, out, err);
	}
};




int main() {
	std::string str;
	std::string input = "";
	WLP4Parser parser;

	while (getline(std::cin, str))
		input += str + '\n';
	parser.parse(input);
}
