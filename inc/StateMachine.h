//------------------------------------------------------------------------------
//
// Copyright (C) 2013 Cody Griffin (cody.m.griffin@gmail.com)
// 

#ifndef INCLUDED_STATEMACHINE_H
#define INCLUDED_STATEMACHINE_H

#include <stdexcept>
#include <memory>

//------------------------------------------------------------------------------

namespace corvid { 

//------------------------------------------------------------------------------

template <typename E>
struct StateHandler {
  virtual void onEvent(E event) {
    throw std::runtime_error("Unable to handle event E");
  }
};

//------------------------------------------------------------------------------

template <typename...Events>
struct Machine  {
  struct State : public StateHandler<Events>... {
    State(Machine* pMachine) 
    : pMachine_(pMachine) {
    }

    virtual ~State() {
    }

    Machine* getMachine() {
      return pMachine_;
    }

  private:
    Machine* pMachine_;  
  };

  Machine(State* pInitialState = 0) 
    : pState_(pInitialState) {
  }

  template <typename E>
  void dispatch(E event) {
    if (pState_) {
      static_cast<StateHandler<E>*>(pState_.get())->onEvent(event);
    }
  }

  template <typename S, typename...X>
  void transition(X...x) {
    // Take the long way to ensure proper order (exit then init)
    pState_.reset(0);
    pState_.reset( new S(this, x...));
  }

  State* getState() {
    return pState_.get();
  }

private:
  std::unique_ptr<State> pState_;
};

//------------------------------------------------------------------------------

} // namespace corvid

//------------------------------------------------------------------------------

#endif
