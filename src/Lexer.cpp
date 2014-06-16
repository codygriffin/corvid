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

template <typename Input, typename State>
struct Automata {
  typedef std::multimap<Input, State>       Transitions;
  typedef std::map     <State, Transitions> StateChanges;
  typedef std::set     <State>              States;
  typedef std::set     <Input>              Inputs;
  typedef std::set     <State>              AcceptableStates;

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

  bool accept () {
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

auto re_symbol = [](char c) -> StringAutomata {
  auto states = StringAutomata::StateChanges({
    {
      { "0", { {c,  "1"} } },
      { "1", { StringAutomata::Transitions({}) } },
    }
  });

  return StringAutomata("0", states, {"1"});
};

auto re_range = [](char a, char b) -> StringAutomata {
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

  return StringAutomata("0", states, {"1"});
};

auto re_union = [&](const StringAutomata& a, const StringAutomata& b) -> StringAutomata {
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

  return StringAutomata("0", states, {"1"});
};

auto re_choice = [&](const StringAutomata& a, const StringAutomata& b) -> StringAutomata {
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

  return StringAutomata("0", states, acceptable);
};

auto re_concat = [&](const StringAutomata& a, const StringAutomata& b) -> StringAutomata {
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

  return StringAutomata(re_a.initial(), states, {"1"});
};

auto re_star = [&](const StringAutomata& a) -> StringAutomata {
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

  return StringAutomata("0", states, {"1"});
};

StringAutomata operator+ (const StringAutomata& a, const StringAutomata& b) {
  return re_concat(a,b);
}

StringAutomata operator| (const StringAutomata& a, const StringAutomata& b) {
  return re_union(a,b);
}

StringAutomata operator>> (const StringAutomata& a, const StringAutomata& b) {
  return re_choice(a,b);
}

StringAutomata operator* (const StringAutomata& a) {
  return re_star(a);
}

StringAutomata match(const std::string& str) {
  StringAutomata automata = re_symbol(str[0]);
  for (size_t i = 1; i < str.length(); i++) {
    automata = automata + re_symbol(str[i]);
  }
  return automata;
}

StringAutomata match(char c) {
  return re_symbol(c);
}

int main () {
  std::cout << "Lexer Test" << std::endl;

  try {
    //auto re        = *(match("cat") | re_symbol('b') | (re_symbol('c') + re_symbol('d')));
    auto skip        = re_symbol(' ') | re_symbol('\n') | re_symbol('\t');
    auto digit       = re_range('0', '9');
    auto lower       = re_range('a', 'z');
    auto upper       = re_range('A', 'Z');
    auto alpha       = lower | upper;
    auto alphanum    = alpha | digit;
    auto integer     = digit + *digit;
    auto identifier  = alpha + *(alphanum);
    auto kw_if       = match("if");
    auto kw_while    = match("while");
    auto oparen      = match('(');
    auto cparen      = match(')');
    auto obrack      = match('[');
    auto cbrack      = match(']');
    auto obrace      = match('{');
    auto cbrace      = match('}');
    
    auto re      = skip 
                >> oparen
                >> cparen
                >> obrace
                >> cbrace
                >> obrack
                >> cbrack
                >> identifier
                >> integer;

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

    std::string test = "if (cat) { dog bird 1234 }";
    auto lexeme_start = test.begin();
    for (auto c = test.begin(); c != test.end(); c++) {
      if (!re_dfa.exec(*c))  {
        if (re_dfa.accept()) {
          std::cout << "lexeme - " << std::string(lexeme_start, c) << std::endl;
          re_dfa.reset();
          lexeme_start = c;
          c--;
        }
      }
    }
    //re_dfa.exec('d');
    //re_dfa.exec('o');
    //re_dfa.exec('g');
    //if (!re_dfa.accept()) {
    //  throw std::runtime_error("not an acceptable state");
    //}
  } catch (std::exception& e) { 
    std::cout << "error: " << e.what() << std::endl;
  }
}
