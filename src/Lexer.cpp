#include "StateMachine.h"

#include <regex>
#include <algorithm>
#include <iostream>

struct Nil   {};

typedef corvid::Machine<
  char,
  Nil
> LexerMachine;

struct Start;
struct Integer;
struct Identifier;

template <char A>        struct Literal;

const char* keyword_if = "if";

struct Start : public LexerMachine::State {
  Start(LexerMachine* pMachine) 
    : LexerMachine::State(pMachine) { 
  }

  void onEvent (char c) {
    std::cout << "Start " << c << std::endl;
    if (c == '(') {
      getMachine()->transition<Literal<'('>>();
    }
    if (std::isdigit(c)) {
      getMachine()->transition<Integer>();
    }
    if (std::isalpha(c)) {
      getMachine()->transition<Identifier>();
    }
  }
};

// [0-9]
struct Integer : public LexerMachine::State {
  Integer(LexerMachine* pMachine) : LexerMachine::State(pMachine) {}
  void onEvent (char c) {
    std::cout << "Integer " << c << std::endl;
  }
};

// [a-z]...
struct Identifier : public LexerMachine::State {
  Identifier(LexerMachine* pMachine) : LexerMachine::State(pMachine) {}
  void onEvent (char c) {
    if (std::islower(c)) {
      std::cout << "lower " << c << std::endl;
    }
  }
};

// A
template <char A>
struct Literal : public LexerMachine::State {
  Literal(LexerMachine* pMachine) : LexerMachine::State(pMachine) {}
  void onEvent (char c) {
    std::cout << "Literal<A> " << c << std::endl;
    //getMachine()->yield(A);
  }
};

struct Lexer : public LexerMachine {
  Lexer(const std::string& s)
    : LexerMachine()
    , begin(s.begin())
    , end(s.end()) {
  }

  std::vector<int> tokenize() {
    tokens.clear();
    skip();
    while (begin != end) {
      dispatch(*begin++);  
      skip();
    }
    return tokens;
  }
  
  bool skip() {
    while (begin != end && std::isspace(*begin)) { begin++; } 
  }

  void yield(int i) {
    tokens.push_back(i);
  }
  
  std::vector<int>            tokens;
  std::string::const_iterator begin;
  std::string::const_iterator end;
};

int main () {
  std::cout << "Lexer Test" << std::endl;
  
  try {
    auto text = std::string("if (acb * V), aw: + a34_");
    Lexer lexer(text);
    lexer.transition<Start>();
    auto tokens = lexer.tokenize();
    for (auto t : tokens) {
      std::cout << "token: " << t << std::endl;
    }
  } catch (std::runtime_error e) { 
    std::cout << "error: " << e.what() << std::endl;
  }
}
