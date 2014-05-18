//------------------------------------------------------------------------------
/*
*  
*  The MIT License (MIT)
* 
*  Copyright (C) 2014 Cody Griffin (cody.m.griffin@gmail.com)
* 
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
* 
*  The above copyright notice and this permission notice shall be included in all
*  copies or substantial portions of the Software.
* 
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*  SOFTWARE.
*/

#include "Utilities.h"

namespace corvid {

//------------------------------------------------------------------------------

inline 
void packListItem(List* pList, List*& l, Scope* pScope) {
  if (pList && pList->get(0)) {
    l = as<List>(pList->get(0)->eval(pScope));
  } else {
    l = nil;
  }
}

inline 
void packListItem(List* pList, std::string& s, Scope* pScope) {
  if (pList && pList->get(0)) {
    s = as<Str>(pList->get(0)->eval(pScope))->str;
  } else {
    s = "";
  }
}

inline 
void packListItem(List* pList, int& i, Scope* pScope) {
  if (pList && pList->get(0)) {
    i = as<Int>(pList->get(0)->eval(pScope))->num;
  } else {
    i = 0;
  }
}

inline 
void packListItem(List* pList, float& i, Scope* pScope) {
  if (pList && pList->get(0)) {
    i = as<Float>(pList->get(0)->eval(pScope))->num;
  } else {
    i = 0;
  }
}

//------------------------------------------------------------------------------

template <int N, typename A, int M> 
struct FillTupleWithList { 
  typedef typename std::tuple_element<N, A>::type ElementN;

  inline static void 
  fill (A& tuple, List* pList, Scope* pScope) {
    packListItem(pList, std::get<N>(tuple), pScope);
    FillTupleWithList<N+1, A, M>::fill(tuple, pList->pTail, pScope);
  }
};

template <typename A, int M> 
struct FillTupleWithList<M, A, M> {
  typedef typename std::tuple_element<M, A>::type ElementM;

  inline static void 
  fill (A& tuple, List* pList, Scope* pScope) {
    packListItem(pList, std::get<M>(tuple), pScope);
  }
};

template <typename A>
inline void 
fillTupleWithList(A& a, List* pList, Scope* pScope) {
  FillTupleWithList<
    0, A, std::tuple_size<A>::value - 1
  >::fill(a, pList, pScope);
}

//------------------------------------------------------------------------------

template <typename...S>
Fun* expose(void (*f)(S...)) {
  return new Fun([=](List* pArgs, Scope* pScope) {
    std::tuple<S...> t;
    fillTupleWithList(t, pArgs, pScope);
    corvid::apply(f, t);
    return nil;
  });
}

template <typename R, typename...S>
Fun* expose(R (*f)(S...)) {
  return new Fun([=](List* pArgs, Scope* pScope) {
    std::tuple<S...> t;
    fillTupleWithList(t, pArgs, pScope);
    return new typename FromNative<R>::Type(corvid::apply(f, t));
  });
}

}
