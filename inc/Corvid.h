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


#include <Utilities.h>
#include <Parsing.h>

#include <iostream>
#include <iomanip>
#include <map>
#include <stack>
#include <queue>
#include <list>
#include <memory>
#include <stdexcept>

#include <sstream>

//------------------------------------------------------------------------------

namespace corvid {

enum TokenTypes {
  // Primitives
  SYM,
  STR,
  INT,
  FLOAT,
  BOOL,

  LIST,
  HEAD,
  TAIL,
  SEQ,
  BEGIN,
  END,
  ITEM,
  ATOM,
  SEXPR,
  PROG,
  DQUOTE,
  QUOTE,
  PAREN,
  BRACE,
};

struct Lexer {
  Rule space;
  Rule digit;
  Rule ualpha;
  Rule lalpha;
  Rule symbol;

  Rule alpha;
  Rule alphanum;
  Rule isymchar;
  Rule symchar;
  Rule strchar;
  Rule digits;
  Rule sign;

  Rule strchars;
  Rule symchars;

  Terminal<0>  dquote;
  Terminal<0>  quote;
  Terminal<0>  oparen;
  Terminal<0>  cparen;
  Terminal<0>  obrace;
  Terminal<0>  cbrace;
  Terminal<0>  osqbrack;
  Terminal<0>  csqbrack;

  Terminal<SYM>    sym;
  Terminal<INT>    num;
  Terminal<FLOAT>  flt;
  Terminal<STR>    str;
  Terminal<LIST>   empty;

  Lexer();
};


struct Scope;
  
struct Atom;
struct List;
struct Expr {
  Expr()         : pAtom(0), pList(0) {}
  Expr(Atom* p)  : pAtom(p), pList(0) {}
  Expr(List* p)  : pAtom(0), pList(p) {}

  Atom* pAtom;
  List* pList;

  virtual void   print () = 0;
  virtual Expr* eval  (Scope* pScope) = 0;
  //virtual Expr* clone () ;
};

struct Sym;
struct Int;
struct Float;
struct Str;
struct Fun;
struct Atom : public Expr {
  Atom()         : Expr(this), pSym(0), pInt(0), pFloat(0), pStr(0), pFun(0) {} 
  Atom(Sym* p)   : Expr(this), pSym(p), pInt(0), pFloat(0), pStr(0), pFun(0) {} 
  Atom(Int* p)   : Expr(this), pSym(0), pInt(p), pFloat(0), pStr(0), pFun(0) {} 
  Atom(Float* p) : Expr(this), pSym(0), pInt(0), pFloat(p), pStr(0), pFun(0) {} 
  Atom(Str* p)   : Expr(this), pSym(0), pInt(0), pFloat(0), pStr(p), pFun(0) {} 
  Atom(Fun* p)   : Expr(this), pSym(0), pInt(0), pFloat(0), pStr(0), pFun(p) {} 

  Sym*   pSym;
  Int*   pInt;
  Float* pFloat;
  Str*   pStr;
  Fun*   pFun;
};

List *nil;
struct List : public Expr {
  List()                  : Expr(this), pHead(0), pTail(0) {}
  List(Expr* p, List* q) : Expr(this), pHead(p), pTail(q) {}

  void print() {
    if (!pHead && !pTail) {
      std::cout << "";
      return;
    }

    std::cout << "( ";
    if (pHead) pHead->print();
    std::cout <<  " . ";
    if (pTail) pTail->print();
    std::cout <<  " )";
  }

  Expr* eval (Scope* pScope);

  Expr* head() const {
    if (pHead) {
      return pHead;
    } else {
      return nil;
    }
  }

  List*  tail() const {
    if (pTail) {
      return pTail;
    } else {
      return nil;
    }
  }

  Expr* get(size_t index) const {
    if (index == 0) {
      return pHead;
    } else {
      return pTail->get(index - 1);
    }
  }

  size_t length() const {
    if (!pTail) {
      return 0;
    } else {
      return pTail->length() + 1;
    }
  }

  void append(Expr* pExpr) {
    if (pTail) {
      pTail->append(pExpr);
    } else {
      pHead = pExpr; 
    }   
  }
    
  void each(std::function<void(Expr* pNode)> fun) {
    if (pHead) {
      fun(pHead);
    }
    if (pTail) {
      pTail->each(fun);
    }
  }

  Expr* pHead;
  List*  pTail;
};



struct Scope {
  Scope(Scope* pParentScope = 0) : pParentScope_(pParentScope) {}
  
  Expr* getValue(const std::string& symbol) const {
   auto it = symbols_.find(symbol);
    if (it == symbols_.end()) {
      if (pParentScope_) {
        return pParentScope_->getValue(symbol);
      } else {
        return getValue("nil");
      }
    }

    return it->second;
  }

  void setValue(const std::string& symbol, Expr* pValue) {
    symbols_[symbol] = pValue;
  }

  Scope* extend() {
    return new Scope(this);
  }

  std::map<std::string, Expr*> symbols_;
  Scope*                        pParentScope_;
};

struct Sym : public Atom {
  Sym(ParseNode* pNode) : Atom(this), sym(pNode->match) {} 

  void print() {
    std::cout << sym;
  }

  Expr* eval (Scope* pScope) {
    return pScope->getValue(sym);
  }

  std::string sym; 
};


struct Float : public Atom {
  Float(ParseNode* pNode) : Atom(this), num(atof(pNode->match.c_str())) {} 
  Float(float num) : Atom(this), num(num) {} 

  void print() { std::cout << num; }

  Expr* eval (Scope* pScope) {
    return this;
  }

  float num;
};

struct Int : public Atom {
  Int(ParseNode* pNode) : Atom(this), num(atoi(pNode->match.c_str())) {} 
  Int(int num) : Atom(this), num(num) {} 

  void print() { std::cout << num; }

  Expr* eval (Scope* pScope) {
    return this;
  }

  int num;
};

struct Str : public Atom {
  Str(const std::string& m = "") : Atom(this), str(m) {} 
  Str(ParseNode* pNode) : Atom(this), str(pNode->match.begin()+1, pNode->match.end()-1) {} 

  void print() { std::cout << str; }

  Expr* eval (Scope* pScope) {
    return this;
  }

  std::string str; 
};

template <typename A>
void fillTupleWithList(A& a, List* pList, Scope* pScope);

template<typename N>
struct FromNative {
};

template<>
struct FromNative<float> {
  typedef Float Type;
};

template<>
struct FromNative<int> {
  typedef Int Type;
};

template<>
struct FromNative<bool> {
  typedef Int Type;
};

template<>
struct FromNative<std::string> {
  typedef Str Type;
};

template<>
struct FromNative<List*> {
  typedef List Type;
};

struct Fun : public Atom {
  Fun(std::function<Expr*(List*, Scope*)> f) : Atom(this), fun(f) {} 

  template <typename...S>
  static Fun* native(void (*f)(S...)) {
    return new Fun([=](List* pArgs, Scope* pScope) {
      std::tuple<S...> t;
      fillTupleWithList(t, pArgs, pScope);
      corvid::apply(f, t);
      return nil;
    });
  }

  template <typename...S>
  static Fun* native(bool (*f)(S...)) {
    return new Fun([=](List* pArgs, Scope* pScope) -> Expr* {
      std::tuple<S...> t;
      fillTupleWithList(t, pArgs, pScope);
      if (corvid::test(f, t)) {
        return new Int(1);
      } else {
        return nil;
      }
    });
  }


  template <typename R, typename...S>
  static Fun* native(R (*f)(S...)) {
    return new Fun([=](List* pArgs, Scope* pScope) {
      std::tuple<S...> t;
      fillTupleWithList(t, pArgs, pScope);
      return new typename FromNative<R>::Type(corvid::apply(f, t));
    });
  }

  void print() {
    std::cout << "<function>";
  }

  Expr* eval (Scope* pScope) {
    return this;
  }

  std::function<Expr*(List*, Scope*)> fun;
};


//------------------------------------------------------------------------------

struct Program {
  
};

//------------------------------------------------------------------------------
// Type checking
template <typename T> T* as(Expr* pExpr) { return nil; }

template <> Fun* as<Fun>(Expr* pExpr) { 
  if (pExpr && pExpr->pAtom && pExpr->pAtom->pFun) {
    return pExpr->pAtom->pFun;
  }

  throw std::runtime_error("Type error: s-expr not a function!");
}

template <> Str* as<Str>(Expr* pExpr) { 
  if (pExpr && pExpr->pAtom && pExpr->pAtom->pStr) {
    return pExpr->pAtom->pStr;
  }

  throw std::runtime_error("Type error: s-expr not a string!");
}

template <> Int* as<Int>(Expr* pExpr) { 
  if (pExpr && pExpr->pAtom && pExpr->pAtom->pInt) {
    return pExpr->pAtom->pInt;
  }

  if (pExpr && pExpr->pAtom && pExpr->pAtom->pFloat) {
    return new Int(pExpr->pAtom->pFloat->num);
  }

  throw std::runtime_error("Type error: s-expr not an int!");
}

template <> Float* as<Float>(Expr* pExpr) { 
  if (pExpr && pExpr->pAtom && pExpr->pAtom->pFloat) {
    return pExpr->pAtom->pFloat;
  }

  throw std::runtime_error("Type error: s-expr not a float!");
}

template <> Sym* as<Sym>(Expr* pExpr) { 
  if (pExpr && pExpr->pAtom && pExpr->pAtom->pSym) {
    return pExpr->pAtom->pSym;
  }

  throw std::runtime_error("Type error: s-expr not a symbol!");
}

template <> List* as<List>(Expr* pExpr) { 
  if (pExpr && pExpr->pList) {
    return pExpr->pList;
  }

  throw std::runtime_error("Type error: s-expr not a list!");
}

template <> Atom* as<Atom>(Expr* pExpr) { 
  if (pExpr && pExpr->pAtom) {
    return pExpr->pAtom;
  }

  throw std::runtime_error("Type error: s-expr not an atom!");
}

template <> Expr* as<Expr>(Expr* pExpr) { 
  if (pExpr) {
    return pExpr;
  }

  throw std::runtime_error("Type error: expected s-expr!");
}

}

