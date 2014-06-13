#include "StateMachine.h"

#include <regex>
#include <algorithm>
#include <iostream>
#include <map>

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
  
  void skip() {
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
  // a+(b|c) 
  typedef std::string Input;
  typedef std::string Token;
  typedef std::string State;
  typedef std::map<Input, State>       Transitions;
  typedef std::map<State, Transitions> States;
  typedef std::tuple<State, States>    Machine;

  auto fsm = States({
    { "even", {
      {"\e", "-"},
      {"0",  "even"}, 
      {"1",  "odd"},
      {"\a", "even"},
    }},
    { "odd",  {
      {"\e", "-"},
      {"1",  "even"},
      {"0",  "odd"},
      {"\a", "odd"},
    }},
  });

  auto fsm_exec = [](Machine& m, Input input) {
    auto& state  = std::get<0>(m);
    auto& states = std::get<1>(m);
    state = states.at(state).at(input);
    std::cout << "input = " << input << ", state = " << state << std::endl;
  };

  auto fsm_accept = [](Machine& m) -> Token {
    auto& state  = std::get<0>(m);
    auto& states = std::get<1>(m);
    return states.at(state).at("\a");
  };

  
  try {
    Machine m = {"even", fsm};
    fsm_exec(m, "0");
    fsm_exec(m, "1");
    fsm_exec(m, "1");
    fsm_exec(m, "1");
    fsm_exec(m, "1");
    fsm_exec(m, "1");
    fsm_exec(m, "0");
    std::cout << " yield = " << fsm_accept(m) << std::endl;

  
    auto text = std::string("if (acb * V), aw: + a34_");
    Lexer lexer(text);
    lexer.transition<Start>();
    auto tokens = lexer.tokenize();
    for (auto t : tokens) {
      std::cout << "token: " << t << std::endl;
    }
  } catch (std::exception& e) { 
    std::cout << "error: " << e.what() << std::endl;
  }
}
