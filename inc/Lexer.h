#include <regex>
#include <algorithm>
#include <numeric>
#include <limits>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <list>
#include <set>
#include <regex> // this could be replaced...

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
  typedef S                                        State;
  typedef std::multimap<Input, State>              Transitions;
  typedef std::map     <State, Transitions>        StateChanges;
  typedef std::set     <State>                     States;
  typedef std::set     <Input>                     Inputs;
  typedef std::set     <State>                     AcceptableStates;
  typedef std::function<std::string(Automata&)>    Action;
  typedef Automata<Input, States>                  Realized;

  Automata(State initialState, StateChanges changes, AcceptableStates acceptableStates) 
    : initialState_     (initialState) 
    , currentState_     (initialState_) 
    , stateChanges_     (changes) 
    , acceptableStates_ (acceptableStates) {
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

  bool accept() const {
    return acceptableStates_.count(currentState_) > 0;
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

  // Get valid inputs for a given state
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
  
  // Get set of states reachable by e-transitions from given states
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
  
  // Get set of states reachable by a input from given states
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
  State        initialState_;
  State        currentState_;
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

  static Regex exclude(char a) {
    auto states = StringAutomata::StateChanges({
      {
        { "1", { StringAutomata::Transitions({}) } },
      }
    });

    for (int i = 1; i <= 255; ++i) {
      if (i != a) {
        states["0"].insert(std::make_pair(i, "1"));
      }
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

template <typename TokenType = std::string>
struct Lexer {
  typedef StringAutomata::Realized                        LexerAutomata;
  typedef std::string                                     Lexeme;
  typedef std::list<std::pair<TokenType, StringAutomata>> LexerSpec;
  typedef std::pair<TokenType, Lexeme>                    Token;

  static Lexer tokens(const LexerSpec& patterns) {
    auto states = StringAutomata::StateChanges({
      {
        { "0", { StringAutomata::Transitions({}) } },
      }
    });

    auto acceptable = StringAutomata::States({});

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

  static std::string getTokenFromState(const std::string& state) {
    std::regex statePattern(":[^\\.]+\\.1:"); 
    std::smatch stateMatch;
    if (std::regex_search(state, stateMatch, statePattern) > 0) {
      auto result = stateMatch[0].str();
      return result.substr(1, result.length()-4);
    }

    return "";
  }

  Token nextToken() {
    if (begin_ != end_) {
      // save the beginning of the lexeme
      auto lexeme_start = begin_;
      
      // go through our automata until we can't
      uint32_t lineNum = 0;
      uint32_t colNum = 0;
      while (automata_.exec(*begin_++)) { 
        // as we go through, keep track of line and col numbers
        if (*begin_ == '\n')  {
          lineNum++;
          colNum = 0;
        } else {
          colNum++;
        }
      }

      // the automata will be left in some state, depending on the input
      auto token = getTokenFromState(stringState(automata_.state()));

      // if its an acceptable state
      if (automata_.accept()) {
        // create the token
        current_ = std::make_pair(token, std::string(lexeme_start, begin_ - 1));
        lineNum_ += lineNum;
        colNum_  += colNum;
        // reset
        automata_.reset();
        // go back a character
        begin_--;

        if (token == "SKIP") { //This is pretty hacky...
          return nextToken();
        } else {
          return current_;
        }
      }     
    }

    std::runtime_error("bad token"); 
    return  std::make_pair("", "");
  }

  Token token() const {
    return current_;
  }

  void consume(std::string token) {
    auto match = nextToken();  
    if (token != match.first) {
      throw std::runtime_error("can't consume");
    } 
  }

  Token lookahead() {
    auto begin = begin_;
    auto old   = current_;
    auto token = nextToken();
    begin_     = begin;
    current_   = old;
    return token;
  }

  bool eof() const { return begin_ == end_; }
  
  void tokenize(const std::string::iterator& begin, const std::string::iterator& end) {
    automata_.reset();
    begin_ = begin;
    end_   = end;
    lineNum_ = 0;
    colNum_  = 0;
  }

  void print() {
    std::cout << automata_.toString() << std::endl;
  }
  
private:
  Lexer(StringAutomata::State initialState, 
        StringAutomata::StateChanges changes, 
        StringAutomata::AcceptableStates acceptableStates) 
    : automata_(StringAutomata(initialState, changes, acceptableStates).realize())
    , end_     (begin_) 
    , current_ (std::make_pair("", "")) {
  }

  LexerAutomata            automata_;

  Lexeme::iterator         begin_; 
  Lexeme::iterator         end_;
  Token                    current_;
  uint32_t                 lineNum_;
  uint32_t                 colNum_;
};

typedef Lexer<std::string> StringLexer;

Regex operator+ (const Regex& a, const Regex& b) {
  return Regex::seq(a,b);
}

Regex operator| (const Regex& a, const Regex& b) {
  return Regex::alt(a,b);
}

Regex operator* (const Regex& a) {
  return Regex::many(a);
}

