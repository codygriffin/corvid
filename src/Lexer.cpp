#include "StateMachine.h"

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

  void exec(Input input) {
    auto transition = stateChanges_.at(currentState_);
    if (transition.count(input) > 1) {
      throw std::runtime_error("need to realize NFA");
    }
    if (transition.count(input) == 0) {
      throw std::runtime_error("illegal state transition");
    }
    auto newState = transition.find(input);
    if (newState != transition.end()) {
      currentState_ = newState->second;
    }
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


  Automata<Input, States> realize() const {
    // 1. Create the start state of the DFA by taking the e-closure of the start state 
    //    of the NFA.
    // 2. Perform the following for the new DFA state: 
    //    For each possible input symbol:
    //    1. Apply move to the newly-created state and the input symbol; this will return 
    //       a set of states.
    //    2. Apply the e-closure to this set of states, possibly resulting in a new set.
    //
    // This set of NFA states will be a single state in the DFA.
    // 3. Each time we generate a new DFA state, we must apply step 2 to it. The process 
    //    is complete when applying step 2 does not yield any new states.
    // 4. The finish states of the DFA are those which contain any of the finish states 
    //    of the NFA.
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
    // The e-closure function takes a state and returns the set of states reachable 
    // from it based on (one or more) e-transitions. Note that this will always 
    // include the state tself. We should be able to get from a state to any state 
    // in its e-closure without consuming any input.
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
    // The function move takes a state and a character, and returns the set of 
    // states reachable by one transition on this character.
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
protected:

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

int main () {
  std::cout << "Lexer Test" << std::endl;

  try {
    auto re     = *(match("cat") | re_symbol('b') | (re_symbol('c') + re_symbol('d')));
    auto re_dfa = re.realize();

    std::cout << re.toString() << std::endl;
    std::cout << re_dfa.toString() << std::endl;
    re_dfa.exec('b');
    re_dfa.exec('c');
    re_dfa.exec('d');
    re_dfa.exec('c');
    re_dfa.exec('d');
    if (!re_dfa.accept()) {
      throw std::runtime_error("not an acceptable state");
    }
  } catch (std::exception& e) { 
    std::cout << "error: " << e.what() << std::endl;
  }
}
