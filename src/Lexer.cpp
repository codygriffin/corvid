#include "StateMachine.h"

#include <regex>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <map>
#include <list>

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

  auto re_namespace = [](const Nfa& nfa, const std::string& name) -> Nfa {
    NfaStates states;
    auto& nstate  = std::get<0>(nfa);
    auto& nstates = std::get<1>(nfa);
    auto& naccept = std::get<2>(nfa);
    for (auto s : nstates) {
      for (auto t : s.second) {
        states[name+s.first][t.first] = {};
        for (auto u : t.second) {
          states[name+s.first][t.first].push_back(name + u);
        }
      }
    }
    std::list<State> accept;
    for (auto a : naccept) {
      accept.push_back(name + a);  
    }
    return Nfa(name + nstate, states, accept); 
  };

  auto re_symbol = [](std::string sym) -> Nfa {
    auto states = NfaStates({
      { "0", {
        {sym,  {"1"}}, 
      }},
      { "1",  {
        {},
      }}
    });

    return Nfa("0", states, {"1"});
  };

  auto re_union = [&](const Nfa& a, const Nfa& b) -> Nfa {
    auto re_a   = re_namespace(a, "a.");
    auto re_b   = re_namespace(b, "b.");

    auto& a_state  = std::get<0>(re_a);
    auto& a_states = std::get<1>(re_a);
    auto& a_accept = std::get<2>(re_a);
    auto& b_state  = std::get<0>(re_b);
    auto& b_states = std::get<1>(re_b);
    auto& b_accept = std::get<2>(re_b);

    auto states = NfaStates({
      { "0", {
        {}, 
      }},
      { "1",  {
        {},
      }}
    });

    // Epsilon transitions
    states["0"][""] = {a_state, b_state};
    for (auto a : a_accept) {
      states[a][""] = {"1"};
    }
    for (auto b : b_accept) {
      states[b][""] = {"1"};
    }

    // Subexpressions
    for (auto s : a_states) {
      states[s.first] = s.second;
    }
    for (auto s : b_states) {
      states[s.first] = s.second;
    }

    return Nfa("0", states, {"1"});
  };

  auto re = re_union(re_symbol("cat"), re_union(re_symbol("dog"), re_symbol("bird")));
  // print stuff
  std::cout << "\n\n NFA: " << std::endl;
  for (auto s : std::get<1>(re)) {
    std::cout << s.first << std::endl;
    for (auto t : s.second) {
      std::cout << "  " << std::setw(6) << t.first << ": ";
      for (auto d : t.second) {
          std::cout << d <<  "  " ;
      }
      std::cout << std::endl;
    }
  }
  
  // http://www.cs.may.ie/staff/jpower/Courses/Previous/parsing/node9.html
  auto closure = [](const Nfa& nfa, const State& state) -> std::list<State> {
    std::list<State> states;
    const auto& nstate  = std::get<0>(nfa);
    const auto& nstates = std::get<1>(nfa);
    const auto& naccept = std::get<2>(nfa);
    // The e-closure function takes a state and returns the set of states reachable 
    // from it based on (one or more) e-transitions. Note that this will always 
    // include the state tself. We should be able to get from a state to any state 
    // in its e-closure without consuming any input.
    states.push_back(state);
    auto state_transitions = std::get<1>(nfa)[state];
    return states;
  };

  auto move = [](const Nfa& nfa, const State& state, Input input) -> std::list<State> {
    std::list<State> states;
    // The function move takes a state and a character, and returns the set of 
    // states reachable by one transition on this character.

    return states;
  };

  auto nfa_to_dfa = [](const Nfa& nfa) -> Dfa {
    Dfa dfa;
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

    return dfa;
  };

  auto re_dfa = nfa_to_dfa(re);
  std::cout << "\n\n DFA: " << std::endl;
  for (auto s : std::get<1>(re_dfa)) {
    std::cout << s.first << std::endl;
    for (auto t : s.second) {
      std::cout << "  " << std::setw(6) << t.first << ": ";
      for (auto d : t.second) {
          std::cout << d <<  "  " ;
      }
      std::cout << std::endl;
    }
  }

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
