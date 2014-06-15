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
    , stateChanges_     (changes) 
    , acceptableStates_ (acceptableStates) {
  }

  State state() const {
    return currentState_;
  }

  StateChanges changes() const {
    return stateChanges_;
  }

  States acceptable() const {
    return acceptableStates_;
  }

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
    return Automata(prefix  + currentState_, states, accept); 
  };


  Automata<Input, States> realize() {
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
    auto dfaState     = closure({currentState_}); //TODO initial state isn't current...
    auto initialState = dfaState;

    std::function<void (States)> simulate = 
    [&](States state) {
      changes[state].insert({}); 
      for (auto i : inputs(state)) {
        auto newState = closure(move(state, i));
        if (i != '\0') {
          changes[state].insert(std::make_pair(i, newState)); 
          simulate(newState);
        }
      }
    };

    simulate(initialState);

    // acceptable states

    return Automata<Input, States>(initialState, changes, acceptable);
  }

  Inputs inputs(States states) {
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

  
  States closure(States states) {
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

  
  States move(States states, Input input) {
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
  StateChanges stateChanges_;
  States       acceptableStates_;
};

typedef Automata<char, std::string> StringAutomata;

int main () {
  std::cout << "Lexer Test" << std::endl;
  typedef std::string                       Input;
  typedef std::string                       Token;
  typedef std::string                       State;

  typedef std::map<Input, std::list<State>> NfaTransitions;
  typedef std::map<State, NfaTransitions>   NfaStates;
  typedef std::tuple<State, NfaStates, std::list<State>>    Nfa;

  typedef std::map<Input, State>            DfaTransitions;
  typedef std::map<State, DfaTransitions>   DfaStates;
  typedef std::tuple<State, DfaStates>      Dfa;

  auto fsm = DfaStates({
    { "even", {
      {"0",  "even"}, 
      {"1",  "odd"},
      {"\a", "even"},
    }},
    { "odd",  {
      {"1",  "even"},
      {"0",  "odd"},
      {"\a", "odd"},
    }},
  });

  auto dfa_exec = [](Dfa& m, Input input) {
    auto& state  = std::get<0>(m);
    auto& states = std::get<1>(m);
    state = states.at(state).at(input);
    std::cout << "input = " << input << ", state = " << state << std::endl;
  };

  auto dfa_accept = [](Dfa& m) -> Token {
    auto& state  = std::get<0>(m);
    auto& states = std::get<1>(m);
    return states.at(state).at("\a");
  };

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

    auto a_state  = re_a.state();
    auto a_states = re_a.changes();
    auto a_accept = re_a.acceptable();
    auto b_state  = re_b.state();
    auto b_states = re_b.changes();
    auto b_accept = re_b.acceptable();

    auto states = StringAutomata::StateChanges({
      {
        { "0", { StringAutomata::Transitions({}) } },
        { "1", { StringAutomata::Transitions({}) } },
      }
    });

    // Epsilon transitions
    states["0"].insert(std::make_pair('\0', a_state));
    states["0"].insert(std::make_pair('\0', b_state));

    // Subexpressions
    for (auto s : a_states) {
      states[s.first] = s.second;
    }
    for (auto s : b_states) {
      states[s.first] = s.second;
    }

    //  Acceptors
    for (auto a : a_accept) {
      states[a].insert(std::make_pair('\0', "1"));
    }
    for (auto b : b_accept) {
      states[b].insert(std::make_pair('\0', "1"));
    }

    return StringAutomata("0", states, {"1"});
  };

  auto re = re_union(re_symbol('a'), re_union(re_symbol('b'), re_symbol('c')));
  //auto re = re_union(re_symbol('a'), re_symbol('b'));
  //auto re = re_symbol('a');

  std::cout << re.toString() << std::endl;
  std::cout << std::endl;
  std::cout << re.realize().toString() << std::endl;

  try {
  /*
    Dfa m = {"even", fsm};
    dfa_exec(m, "0");
    dfa_exec(m, "1");
    dfa_exec(m, "1");
    dfa_exec(m, "1");
    dfa_exec(m, "1");
    dfa_exec(m, "1");
    dfa_exec(m, "0");
    std::cout << " yield = " << dfa_accept(m) << std::endl;
  */
  } catch (std::exception& e) { 
    std::cout << "error: " << e.what() << std::endl;
  }
}
