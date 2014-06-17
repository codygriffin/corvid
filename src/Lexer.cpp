#include <regex>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <map>
#include <list>
#include <set>

template <typename State>
std::string stringState(State state); 

template <>
std::string stringState<std::string>(std::string state) {
  return state;
} 

template <>
std::string stringState<std::set<std::string>>(std::set<std::string> states) {
  std::string newState = ":";
  for (auto state : states) {
    newState += state + ":";
  }
  return newState;
} 

template <typename Input, typename S>
struct Automata {
  typedef S                                 State;
  typedef std::multimap<Input, State>       Transitions;
  typedef std::map     <State, Transitions> StateChanges;
  typedef std::set     <State>              States;
  typedef std::set     <Input>              Inputs;
  typedef std::set     <State>              AcceptableStates;
  typedef std::function<std::string(Automata&)>    Action;

  Automata(State initialState, StateChanges changes, AcceptableStates acceptableStates) 
    : currentState_     (initialState) 
    , initialState_     (initialState) 
    , stateChanges_     (changes) 
    , acceptableStates_ (acceptableStates) {
  }

  Automata() 
    : currentState_     ("") 
    , initialState_     ("") 
    , stateChanges_     ({}) 
    , acceptableStates_ ({}) {
  }

  State state() const {
    return currentState_;
  }

  State initial() const {
    return initialState_;
  }

  StateChanges changes() const {
    return stateChanges_;
  }

  States acceptable() const {
    return acceptableStates_;
  }

  void reset() {
    currentState_ = initialState_;
  }
  
  bool exec(Input input) {
    auto transition = stateChanges_.at(currentState_);
    if (transition.count(input) > 1) {
      throw std::runtime_error("need to realize NFA");
    }

    if (transition.count(input) == 0) {
      return false;
    }

    auto newState = transition.find(input);
    currentState_ = newState->second;
    return true;
  };

  bool accept() {
    if (acceptableStates_.count(currentState_) > 0) {
      return true;
    }

    return false;
  };
  
  std::string toString() const {
    std::stringstream str;

    for (auto s : stateChanges_) {
      str << "state: " << stringState(s.first) << std::endl;
      for (auto t : s.second) {
        str << "\t  on \'"  << t.first  << "\' ";
        str << "goto " << stringState(t.second) << std::endl;
      }
    }
    str << "acceptable: " << std::endl;
    for (auto s : acceptableStates_) {
      str << "\t  " << stringState(s) << std::endl;
    }

    return str.str();
  }

  Automata prefix(const std::string& prefix) const {
    StateChanges states;
    for (auto s : stateChanges_) {
      for (auto t : s.second) {
        states[prefix + s.first].insert(std::make_pair(t.first, prefix + t.second));
      }
    }
    AcceptableStates accept;
    for (auto a : acceptableStates_) {
      accept.insert(prefix + a);  
    }

    return Automata(prefix  + initialState_, states, accept); 
  };

  // Really effin' slow right now
  Automata<Input, States> realize() const {
    typename Automata<Input, States>::StateChanges changes;
    typename Automata<Input, States>::States       acceptable;
    auto dfaState     = closure({initialState_}); 
    auto initialState = dfaState;

    std::function<void (States)> simulate = [&](States state) {
      if (!changes.count(state)) {
        changes[state].insert({}); 
        for (auto i : inputs(state)) {
          auto newState = closure(move(state, i));
          if (i != '\0') {
            changes[state].insert(std::make_pair(i, newState)); 
            simulate(newState);
          }
        }
      }
    };

    simulate(initialState);

    for (auto a : acceptableStates_) {
      for (auto s : changes) {
        if (s.first.count(a)) {
          acceptable.insert(s.first);
        }
      }
    }
    // acceptable states

    return Automata<Input, States>(initialState, changes, acceptable);
  }

  Inputs inputs(States states) const {
    Inputs inputStates;
    
    for (auto state : states) {
      auto transitions        = stateChanges_.find(state);
      if (transitions != stateChanges_.end()) {
        for (auto transition : transitions->second) {
          inputStates.insert(transition.first);
        }
      }
    }
    return inputStates;
  }
  
  States closure(States states) const {
    States closureStates = states;
    for (auto state : states) {
      auto transitions        = stateChanges_.find(state);
      if (transitions != stateChanges_.end()) {
        auto epsilonTransitions = transitions->second.equal_range('\0');
        for (auto epsilon  = epsilonTransitions.first; 
                  epsilon != epsilonTransitions.second;
                ++epsilon) {
          if (!closureStates.count(epsilon->second)) {
            auto nextStates = closure({epsilon->second});
            closureStates.insert(nextStates.begin(), nextStates.end());
          }
        }
      }
    }

    return closureStates;
  }
  
  States move(States states, Input input) const {
    States moveStates;
    for (auto state : states) {
      auto transitions = stateChanges_.find(state);
      if (transitions != stateChanges_.end()) {
        auto matches     = transitions->second.equal_range(input);
        for (auto transition  = matches.first; 
                  transition != matches.second;
                ++transition) {
          moveStates.insert(transition->second);
        }
      }
    }

    return closure(moveStates);
  };

private:
  State        currentState_;
  State        initialState_;
  StateChanges stateChanges_;
  States       acceptableStates_;
};

typedef Automata<char, std::string> StringAutomata;

struct Regex : public StringAutomata {

  static Regex symbol(char c) {
    auto states = StringAutomata::StateChanges({
      {
        { "0", { {c,  "1"} } },
        { "1", { StringAutomata::Transitions({}) } },
      }
    });

    return Regex("0", states, {"1"});
  };

  static Regex range(char a, char b) {
    char min_a = std::min(a, b);
    char max_b = std::max(a, b);
    auto states = StringAutomata::StateChanges({
      {
        { "1", { StringAutomata::Transitions({}) } },
      }
    });

    for (; min_a <= max_b; min_a++) {
      states["0"].insert(std::make_pair(min_a, "1"));
    }

    return Regex("0", states, {"1"});
  };

  static Regex alt(const Regex& a, const Regex& b) {
    auto re_a   = a.prefix("a.");
    auto re_b   = b.prefix("b.");

    auto states = StringAutomata::StateChanges({
      {
        { "0", { StringAutomata::Transitions({}) } },
        { "1", { StringAutomata::Transitions({}) } },
      }
    });

    // Epsilon transitions
    states["0"].insert(std::make_pair('\0', re_a.initial()));
    states["0"].insert(std::make_pair('\0', re_b.initial()));

    // Subexpressions
    for (auto s : re_a.changes()) {
      states[s.first] = s.second;
    }
    for (auto s : re_b.changes()) {
      states[s.first] = s.second;
    }

    //  Acceptors
    for (auto a : re_a.acceptable()) {
      states[a].insert(std::make_pair('\0', "1"));
    }
    for (auto b : re_b.acceptable()) {
      states[b].insert(std::make_pair('\0', "1"));
    }

    return Regex("0", states, {"1"});
  };

  static Regex seq(const Regex& a, const Regex& b) {
    auto re_a   = a.prefix("a.");
    auto re_b   = b.prefix("b.");

    auto a_states = re_a.changes();
    auto b_states = re_b.changes();

    auto states = StringAutomata::StateChanges({}); 
    states.insert(a_states.begin(), a_states.end());
    states.insert(b_states.begin(), b_states.end());

    for (auto a : re_a.acceptable()) {
      states[a].insert(std::make_pair('\0', re_b.initial()));
    }

    for (auto b : re_b.acceptable()) {
      states[b].insert(std::make_pair('\0', "1"));
    }

    return Regex(re_a.initial(), states, {"1"});
  };

  static Regex many(const Regex& a) {
    auto re_a   = a.prefix("a.");

    auto states = StringAutomata::StateChanges({
      {
        { "0", { StringAutomata::Transitions({}) } },
        { "1", { StringAutomata::Transitions({}) } },
      }
    });

    // Epsilon transitions
    states["0"].insert(std::make_pair('\0', re_a.initial()));
    states["0"].insert(std::make_pair('\0', "1"));

    // Subexpressions
    for (auto s : re_a.changes()) {
      states[s.first] = s.second;
    }

    //  Acceptors
    for (auto a : re_a.acceptable()) {
      states[a].insert(std::make_pair('\0', "1"));
      states[a].insert(std::make_pair('\0', re_a.initial()));
    }

    return Regex("0", states, {"1"});
  };

  static Regex match(const std::string& str) {
    Regex automata = Regex::symbol(str[0]);
    for (size_t i = 1; i < str.length(); i++) {
      automata = Regex::seq(automata, Regex::symbol(str[i]));
    }
    return automata;
  }

  static Regex match(char c) {
    return Regex::symbol(c);
  }

private:
  Regex(StringAutomata::State initialState, StringAutomata::StateChanges changes, StringAutomata::AcceptableStates acceptableStates) 
    : StringAutomata(initialState, changes, acceptableStates) {
  }
};


// Flatten this out (like range)
template <typename Token>
struct Lexer : public StringAutomata {
  static Lexer tokens(const StringAutomata& a, const StringAutomata& b) {
    auto re_a   = a.prefix("a.");
    auto re_b   = b.prefix("b.");

    auto states = StringAutomata::StateChanges({
      {
        { "0", { StringAutomata::Transitions({}) } },
      }
    });

    // Epsilon transitions
    states["0"].insert(std::make_pair('\0', re_a.initial()));
    states["0"].insert(std::make_pair('\0', re_b.initial()));

    // Subexpressions
    for (auto s : re_a.changes()) {
      states[s.first] = s.second;
    }
    for (auto s : re_b.changes()) {
      states[s.first] = s.second;
    }

    auto acceptable = StringAutomata::States({});
    for (auto a : re_a.acceptable()) {
      acceptable.insert(a);
    }

    for (auto b : re_b.acceptable()) {
      acceptable.insert(b);
    }

    return Lexer("0", states, acceptable);
  }

  static Lexer tokens(const std::list<std::pair<Token, StringAutomata>>& patterns) {
    auto states = StringAutomata::StateChanges({
      {
        { "0", { StringAutomata::Transitions({}) } },
      }
    });

    auto acceptable = StringAutomata::States({
    });

    int i = 1;
    for (auto p : patterns) {
      auto re_a   = p.second.prefix(p.first + ".");

      // Epsilon transitions
      states["0"].insert(std::make_pair('\0', re_a.initial()));

      for (auto s : re_a.changes())    { states[s.first] = s.second; }
      for (auto a : re_a.acceptable()) { acceptable.insert(a); }
    }

    return Lexer("0", states, acceptable);
  }
  
private:
  Lexer(StringAutomata::State initialState, 
        StringAutomata::StateChanges changes, 
        StringAutomata::AcceptableStates acceptableStates) 
    : StringAutomata(initialState, changes, acceptableStates) {
  }
};

Regex operator+ (const Regex& a, const Regex& b) {
  return Regex::seq(a,b);
}

Regex operator| (const Regex& a, const Regex& b) {
  return Regex::alt(a,b);
}

Regex operator* (const Regex& a) {
  return Regex::many(a);
}

//template <typename T>
//Lexer operator >> (const StringAutomata& a, const StringAutomata& b) {
//  return Lexer<T>::tokens(a,b);
//}

int main () {
  std::cout << "Lexer Test" << std::endl;

  auto skip        = Regex::symbol(' ') | Regex::symbol('\n') | Regex::symbol('\t');
  auto digit       = Regex::range('0', '9');
  auto lower       = Regex::range('a', 'z');
  auto upper       = Regex::range('A', 'Z');
  auto alpha       = lower | upper;
  auto alphanum    = alpha | digit;
  auto number      = digit + *digit;
  auto identifier  = alpha + *(alphanum);

  try {
    auto re = Lexer<std::string>::tokens({
    {"",            skip},
    {"ID",          identifier},
    {"SYM",         Regex::match(":") + identifier},
    {"NUM",         number},
    {"IF",          Regex::match("if")},
    {"WHILE",       Regex::match("while")},
    {"UNTIL",       Regex::match("until")},
    {"LOOP",        Regex::match("loop")},
    {"WHEN",        Regex::match("when")},
    {"DO",          Regex::match("do")},
    {"VAL",         Regex::match("val")},
    {"LET",         Regex::match("let")},
    {"OPAREN",      Regex::match('(')},
    {"CPAREN",      Regex::match(')')},
    {"OBRACK",      Regex::match('[')},
    {"CBRACK",      Regex::match(']')},
    {"OBRACE",      Regex::match('{')},
    {"CBRACE",      Regex::match('}')},
    {"EQUAL",       Regex::match('=')},
    {"PLUS",        Regex::match('+')},
    {"SEMI",        Regex::match(';')},
    {"COLON",       Regex::match(':')},
    {"YIELDS",      Regex::match("->")},
    });

    //std::cout << "#----- DFA" << std::endl;
    //std::cout << re.toString() << std::endl;
    std::cout << "realizing...";
    std::cout.flush();
    auto re_dfa  = re.realize();
    std::cout << "done." << std::endl;

    //std::cout << "#----- NFA" << std::endl;
    //std::cout << re_dfa.toString() << std::endl;

    std::cout << "minimizing...";
    std::cout << "TO BE IMPLEMENTED...";
    std::cout << "done." << std::endl;

    std::string test = 
      "let add2 = (a : u8) -> u8{\n"
      "  1 + 1\n"
      "};\n"
      "\n"
      "val abcd = :abcd;\n";
    auto lexeme_start = test.begin();
    std::string::iterator c;
    for (c = test.begin(); c != test.end(); c++) {
      if (!re_dfa.exec(*c))  {
        std::string state = stringState(re_dfa.state());
        auto dot   = state.find('.');
        auto token = state = state.substr(1, dot-1);
        if (re_dfa.accept()) {
          if (token != "") {
            std::cout << " lexeme - " << std::string(lexeme_start, c)
                      << " token  - " << token << std::endl;
          }
          re_dfa.reset();
          lexeme_start = c;
          c--;
        }
        else {
          std::runtime_error("bad token"); 
        }
      }
    }

    // One last time
    std::string state = stringState(re_dfa.state());
    auto dot   = state.find('.');
    auto token = state = state.substr(1, dot-1);
    if (re_dfa.accept()) {
      if (token != "") {
        std::cout << " lexeme - " << std::string(lexeme_start, c)
                  << " token  - " << token << std::endl;
      } 
      re_dfa.reset();
      lexeme_start = c;
      c--;
    } else {  
      std::runtime_error("bad token");
    }
  } catch (std::exception& e) { 
    std::cout << "error: " << e.what() << std::endl;
  }
}
