#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
const std::string ALPHABET    = ".ALPHABET";
const std::string STATES      = ".STATES";
const std::string TRANSITIONS = ".TRANSITIONS";
const std::string INPUT       = ".INPUT";
const std::string EMPTY       = ".EMPTY";


bool isChar(std::string s) {
	return s.length() == 1;
}
bool isRange(std::string s) {
	return s.length() == 3 && s[1] == '-';
}




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
		std::string name;
		bool accepting;
		std::map<char,State*> next;
	  public:
		/** State constructor must know state's name and accepting condition **/
		State(std::string s, bool b) : name{s}, accepting{b}, next() {}
		void addTransition(char c, State *nextState) {
			next[c] = nextState;
		}
		/** Follow an edge to a connecting node and return the node, if possible **/
		State *transition(char c) {
			return (next.count(c) > 0) ? next[c] : nullptr;
		}
		/** Return state's name **/
		std::string getName() {
			return name;
		}
		/** Return accepting condition **/
		bool isAccepting() {
			return accepting;
		}
	};

	std::map<std::string, State*> states;
	State *start;
  public:
	/** DFA constructor must contain the start state at least **/
	DFA(std::string s, bool b) : states(), start{new State(s, b)} {
		states[s] = start;
	}
	/** Delete all memory from states owned by the DFA **/
	~DFA() {
		for (auto &kv : states) {
			delete kv.second;
		}
	}

	/** Create another state in the DFA (initially with no edges) **/
	void addState(std::string s, bool b) {
		if (states.count(s) == 0)
			states[s] = new State(s, b);
	}
	/** Create a transition (conditional, unidirectional edge) between two states **/
	void addTransition(std::string from, char c, std::string to) {
		if (states.count(from) > 0 && states.count(to) > 0)
			states[from]->addTransition(c, states[to]);
	}

	/** From A4P2 - previous Input section, replacing with SMM **/
	void originalInput(std::istream &in = std::cin, std::ostream &out = std::cout) {
		std::string s;

		while(in >> s) {
			//// Variable 's' contains an input string for the DFA
			State *curState = start;
			bool accepted = false;
			if (s != EMPTY) {
				for (char c : s) {
					curState = curState->transition(c);
					if (curState == nullptr) break;
				}
			}
			accepted = (curState != nullptr && curState->isAccepting());
			out << s << ((accepted) ? " true" : " false") << std::endl;
		}
	}
	/** For A4P3 - Simplified Maximal Munch **/
	void smm(std::istream &in = std::cin, std::ostream &out = std::cout, std::ostream &err = std::cerr) {
		std::string s;
		std::string lex = "";
		std::getline(in, s);
		int k = s.size();

		int i = 0;
		State *curState = start;
		State *nextState;
		while(true) {
			nextState = (i < k) ? curState->transition(s[i]) : nullptr;
			if (nextState == nullptr) {
				if (!curState->isAccepting()) {
					err << "ERROR: Unaccepted token attempt - " << lex << std::endl;
					return;
				}
				// already assuming no whitespace in s
				out << lex << std::endl;
				lex = "";
				curState = start;
				if (i == k) return;
			} else {
				lex += s[i++];
				curState = nextState;
			}
		}
	}
};




class WLP4Scanner : public DFA {
	// WLP4Scanner class *is* a DFA, specifically and extensively defined for the WLP4 language specifications

	void whitespace() {
		addState("WHITESPACE", true);

		addTransition("_START", ' ', "WHITESPACE");
		addTransition("_START", '\t', "WHITESPACE");
		addTransition("WHITESPACE", ' ', "WHITESPACE");
		addTransition("WHITESPACE", '\t', "WHITESPACE");
	}
	void delimiters() {
		addState("LPAREN", true);
		addState("RPAREN", true);
		addState("LBRACE", true);
		addState("RBRACE", true);
		addState("LBRACK", true);
		addState("RBRACK", true);

		addTransition("_START", '(', "LPAREN");
		addTransition("_START", ')', "RPAREN");
		addTransition("_START", '{', "LBRACE");
		addTransition("_START", '}', "RBRACE");
		addTransition("_START", '[', "LBRACK");
		addTransition("_START", ']', "RBRACK");
	}
	void relationals() {
		addState("BECOMES", true);
		addState("EQ", true);
		addState("LT", true);
		addState("LE", true);
		addState("GT", true);
		addState("GE", true);
		addState("_NOT", false);
		addState("NE", true);

		addTransition("_START", '=', "BECOMES");
		addTransition("BECOMES", '=', "EQ");

		addTransition("_START", '<', "LT");
		addTransition("LT", '=', "LE");

		addTransition("_START", '>', "GT");
		addTransition("GT", '=', "GE");

		addTransition("_START", '!', "_NOT");
		addTransition("_NOT", '=', "NE");
	}
	void opsandpunctuation() {
		addState("PLUS", true);
		addState("MINUS", true);
		addState("STAR", true);
		addState("SLASH", true);
		addState("PCT", true);
		addState("COMMA", true);
		addState("SEMI", true);
		addState("AMP", true);
		addState("COMMENT", true);

		addTransition("_START", '+', "PLUS");
		addTransition("_START", '-', "MINUS");
		addTransition("_START", '*', "STAR");
		addTransition("_START", '/', "SLASH");
		addTransition("_START", '%', "PCT");
		addTransition("_START", ',', "COMMA");
		addTransition("_START", ';', "SEMI");
		addTransition("_START", '&', "AMP");

		addTransition("SLASH", '/', "COMMENT");
	}
	void numbers() {
		addState("ZERO", true);
		addState("NUM", true);

		addTransition("_START", '0', "ZERO");
		for (char c = '1'; c <= '9'; ++c) addTransition("_START", c, "NUM");
		for (char c = '0'; c <= '9'; ++c) addTransition("NUM", c, "NUM");
	}
	void identifiers() {
		// just read all IDs and keywords as is, then distinguish kind at SMM
		addState("ID", true);

		for (char c = 'a', C = 'A'; c <= 'z'; ++c, ++C) {
			addTransition("_START", c, "ID");
			addTransition("_START", C, "ID");
			addTransition("ID", c, "ID");
			addTransition("ID", C, "ID");

		}
		for (char d = '0'; d <= '9'; ++d) {
			addTransition("ID", d, "ID");
		}
	}
	std::string getKind(std::string stateName, std::string lex) {
		if (stateName == "ZERO") {
			return "NUM";
		} else if (lex == "return") {
			return "RETURN";
		} else if (lex == "if") {
			return "IF";
		} else if (lex == "int") {
			return "INT";
		} else if (lex == "else") {
			return "ELSE";
		} else if (lex == "wain") {
			return "WAIN";
		} else if (lex == "while") {
			return "WHILE";
		} else if (lex == "println") {
			return "PRINTLN";
		} else if (lex == "new") {
			return "NEW";
		} else if (lex == "delete") {
			return "DELETE";
		} else if (lex == "NULL") {
			return "NULL";
		} else {
			return stateName;
		}
	}
  public:
	WLP4Scanner() : DFA("_START", false) {
		whitespace();
		delimiters();
		relationals();
		opsandpunctuation();
		numbers();
		identifiers();
	}
	void scanAll(std::istream &in = std::cin, std::ostream &out = std::cout, std::ostream &err = std::cerr) {
		// modified version of the simplified maximal munch algorithm
		std::string s;

		while (std::getline(in, s)) {
			if (s.length() == 0) continue;
			std::string kind = "";
			std::string lex = "";

			int k = s.size();
			int i = 0;
			State *curState = start;
			State *nextState;
			while(true) {
				nextState = (i < k) ? curState->transition(s[i]) : nullptr;
				if (nextState == nullptr) {
					if (!curState->isAccepting()) {
						err << "ERROR: Unaccepted token attempt - " << lex << std::endl;
						return;
					}

					kind = getKind(curState->getName(), lex);

					if (kind == "NUM" && lex.length() > 9) {
						if (lex.length() != 10 || lex.compare("2147483647") > 0) {
							err << "ERROR: Number out of bounds --> " << lex << std::endl;
							return;
						}
					}
					if (kind == "COMMENT") break;
					if (kind != "WHITESPACE") out << kind << ' ' << lex << std::endl;
					if (i == k) break;

					lex = "";
					curState = start;
				} else {
					lex += s[i++];
					curState = nextState;
				}
			}
		}
	}
};




// Define the specific DFA for the WLP4 language specs, then use it to
// scan the provided WLP4 file input and produce tokens using the full
// simplified maximal munch algorithm
int main() {
	WLP4Scanner scanner;
	scanner.scanAll();
}
