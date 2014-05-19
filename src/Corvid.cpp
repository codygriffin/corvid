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

#include "Corvid.h"

#include <cmath>
#include <fstream>

using namespace corvid;

//------------------------------------------------------------------------------

Expr* evalProcForm(List* pList, Scope* pScope) {
  pScope = pScope->extend();
  auto pHead = pList->pHead->eval(pScope);
  return as<Fun>(pHead)->fun(pList->pTail, pScope);
}

Expr* makeLambda(List* pArgs, Expr* pLambda, Scope* pScope) {

  // Create a new procedure that evaluates the lambda expression
  return new Fun([=](List* Args, Scope* Scope) {
    // Create a new scope for our lambda
    auto pClosure = Scope->extend();

    int arg = 0;
    Args->each ([&](Expr* pExpr) {
      auto pArgValue = pExpr->eval(pClosure);
      pClosure->setValue(as<Sym>(pArgs->get(arg))->sym, pArgValue);
      arg++;
    });

    return pLambda->eval(pClosure);
  });
}

Expr* evalLambdaForm(List* pList, Scope* pScope) {
  auto pLambdaArgs = as<List>(pList->get(1));
  auto pLambdaExpr = as<Expr>(pList->get(2));
  return makeLambda(pLambdaArgs, pLambdaExpr, pScope);
}

Expr* evalDefineForm(List* pList, Scope* pScope) {
  if (pList->get(1)->pAtom) {
    auto pName  = as<Sym>(pList->get(1));
    auto pValue = pList->get(2)->eval(pScope);
    pScope->setValue(pName->sym, pValue);
    return pValue;
  } else  {
    auto pArgs       = as<List>(pList->get(1));
    auto pLambdaExpr = as<List>(pList->get(2));
    auto pName       = as<Sym> (pArgs->pHead);
    auto pLambdaArgs = as<List>(pArgs->pTail);
    auto pLambda     = makeLambda(pLambdaArgs, pLambdaExpr, pScope);
    pScope->setValue(pName->sym, pLambda);
    return pLambda;
  }
}

Expr* evalQuoteForm(List* pList, Scope* pScope) {
  return pList->get(1);
}

Expr* evalIfForm(List* pList, Scope* pScope) {
  auto pCondition = pList->get(1)->eval(pScope);

  if (!pCondition->pList) {
    return pList->get(2)->eval(pScope);
  } else {
    return pList->get(3)->eval(pScope);
  }
}

Expr* evalList(List* pList, Scope* pScope) {
  return pList->eval(pScope);
}

Expr* List::eval (Scope* pScope) {
  // If our list has no first element 
  if (!get(0)) {
    return this;
  }

  if (get(0) && get(0)->pAtom && get(0)->pAtom->pSym) {
    auto pSym = get(0)->pAtom->pSym;
    if (pSym->sym == "define") {
      return evalDefineForm(this, pScope);
    }
    if (pSym->sym == "lambda" || pSym->sym == ".\\") {
      return evalLambdaForm(this, pScope);
    }
    if (pSym->sym == "quote") {
      return evalQuoteForm(this, pScope);
    }
    if (pSym->sym == "if") {
      return evalIfForm(this, pScope);
    }
  }

  return evalProcForm(this, pScope);
}

std::unique_ptr<Scope> pGlobalScope;

#include "Bindings.h"

int opAdd(int a, int b) { return a + b; }
int opSub(int a, int b) { return a - b; }
int opMul(int a, int b) { return a * b; }
int opDiv(int a, int b) { return a / b; }
int opMod(int a, int b) { return a % b; }
float opSin(float a)    { return sin(a); }

bool opEq(int a, int b) { return a == b; }
bool opEqs(std::string a, std::string b) { return a == b; }
bool opLt(int a, int b) { return a <  b; }
bool opGt(int a, int b) { return a >  b; }

void load(std::string path);

int len  (List* pArgs) {
    return pArgs->length();
}

std::string fill  (int n, List* pArgs) {
    return "implemented"; 
}

void dumpEnv(int n) {
  for (auto sym : pGlobalScope->symbols_) {
    std::cout << sym.first << " = ";
    sym.second->print();
    std::cout << std::endl;
  }
}

void initGlobalScope() {
  pGlobalScope.reset(new Scope(0));
  nil = new List();
  
  pGlobalScope->setValue("+",       Fun::native(&opAdd)); 
  pGlobalScope->setValue("-",       Fun::native(&opSub)); 
  pGlobalScope->setValue("*",       Fun::native(&opMul)); 
  pGlobalScope->setValue("/",       Fun::native(&opDiv)); 
  pGlobalScope->setValue("%",       Fun::native(&opMod)); 
  pGlobalScope->setValue("=",       Fun::native(&opEq)); 
  pGlobalScope->setValue("s=",       Fun::native(&opEqs)); 
  pGlobalScope->setValue("<",       Fun::native(&opLt)); 
  pGlobalScope->setValue(">",       Fun::native(&opGt)); 
  pGlobalScope->setValue("sin",     Fun::native(&opSin)); 
  pGlobalScope->setValue("len",     Fun::native(&len)); 
  pGlobalScope->setValue("fill",    Fun::native(&fill)); 
  pGlobalScope->setValue("dumpenv", Fun::native(&dumpEnv)); 
  pGlobalScope->setValue("load",    Fun::native(&load)); 

  pGlobalScope->setValue("cons",  new Fun([](List* pArgs, Scope* pScope) {
    auto pHead = pArgs->get(0)->eval(pScope);
    auto pTail = pArgs->get(1)->eval(pScope)->pList;
    return new List(pHead, pTail);
  }));
  pGlobalScope->setValue("head",  new Fun([](List* pArgs, Scope* pScope) {
    return pArgs->get(0)->eval(pScope)->pList->head();
  }));
  pGlobalScope->setValue("tail",  new Fun([](List* pArgs, Scope* pScope) {
    return pArgs->get(0)->eval(pScope)->pList->tail();
  }));
  pGlobalScope->setValue("list",  new Fun([](List* pArgs, Scope* pScope) {
    return pArgs;
  }));
  pGlobalScope->setValue("len",  new Fun([](List* pArgs, Scope* pScope) {
    auto len = pArgs->get(0)->eval(pScope)->pList->length();
    return new Int(len);
  }));
  pGlobalScope->setValue("nth",  new Fun([](List* pArgs, Scope* pScope) {
    auto i     = pArgs->get(0)->eval(pScope)->pAtom->pInt->num;
    auto pList = pArgs->get(1)->eval(pScope)->pList; 
    return pList->get(i);
  }));
  pGlobalScope->setValue("nil?",  new Fun([](List* pArgs, Scope* pScope) -> Expr* {
    auto len = pArgs->get(0)->eval(pScope)->pList->length();
    bool result = len == 0; 
    if (result) {
      return new Int(1);
    } else {
      return nil;
    }
  }));
  pGlobalScope->setValue("set!",  new Fun([](List* pArgs, Scope* pScope) {
    auto pName  = pArgs->get(0)->pAtom->pSym;
    auto pValue = pArgs->get(1)->eval(pScope); 
    pScope->setValue(pName->sym, pValue);
    return pValue;
  }));
  pGlobalScope->setValue("print",  new Fun([](List* pArgs, Scope* pScope) {
    while (pArgs) {
      auto pValue = pArgs->pHead;
      if (pValue) {
        pValue = pValue->eval(pScope);  
        pValue->print();
      }
      pArgs = pArgs->pTail;  
    }
    return nil;
  }));
  pGlobalScope->setValue("println",  new Fun([](List* pArgs, Scope* pScope) {
    while (pArgs) {
      auto pValue = pArgs->pHead;
      if (pValue) {
        pValue = pValue->eval(pScope);  
        pValue->print();
      }
      pArgs = pArgs->pTail;  
    }
    std::cout << std::endl;
    return nil;
  }));
  pGlobalScope->setValue("prompt",  new Fun([](List* pArgs, Scope* pScope) {
    std::string input;
    std::getline(std::cin, input);
    return new Str(input);
  }));
  pGlobalScope->setValue("begin",  new Fun([](List* pArgs, Scope* pScope) {
    Expr* pValue = 0;
    while (pArgs) {
      auto pItem = pArgs->pHead;
      if (pItem) {
        pValue = pItem->eval(pScope);  
      }
      pArgs = pArgs->pTail;  
    }
    return pValue;
  }));
  pGlobalScope->setValue("nil", nil);
};

Expr* buildExpr(ParseNode* pNode);

List*  buildListFromTail (ParseNode* pNode) {
  if (!pNode) return nil;

  auto pLeft  = pNode->pLeft.get();
  auto pRight = pNode->pRight.get();

  if (pLeft && pLeft->context == ITEM) {
    return new List(buildExpr(pLeft), buildListFromTail(pRight));
  } else {
    return buildListFromTail(pRight);
  }
}

Expr* buildExpr(ParseNode* pNode) {
  if (!pNode) {
    throw std::runtime_error("cannot build expression from null node");
  }

  // build terminal nodes
  if (pNode->context == SYM) { 
    return new Sym(pNode);
  }

  if (pNode->context == STR) { 
    return new Str(pNode);
  }

  if (pNode->context == INT) { 
    return new Int(pNode);
  }

  if (pNode->context == FLOAT) { 
    return new Float(pNode);
  }

  // recursively build list
  if (pNode->context == LIST) {
    return buildExpr(pNode->pRight.get());
  }

  if (pNode->context == TAIL) {
    return buildListFromTail(pNode);
  }
  
  // If we only have a left side...
  if (pNode->pLeft.get() && !pNode->pRight.get()) {
    return buildExpr(pNode->pLeft.get());
  }

  // If we only have a right side...
  if (!pNode->pLeft.get() && pNode->pRight.get()) {
    return buildExpr(pNode->pRight.get());
  }

  throw std::runtime_error("cannot build expression from unknown node");
}

Lexer::Lexer() { 
  space     = skip(' ', '\n');
  digit     = range('0', '9');
  ualpha    = range('A', 'Z');
  lalpha    = range('a', 'z');
  symbol    = any("+-*/%:;.$,?!|=<>~#\\");

  alpha     = ualpha   | lalpha;
  alphanum  = alpha    | digit;
  isymchar  = alpha    | symbol;
  symchar   = alphanum | symbol;
  strchar   = alphanum | symbol | lit(' ');
  digits    = digit    + *digits;

  strchars  = ~(strchar + *strchars);
  symchars  = ~(symchar + *symchars);

  dquote    = lit('\"');
  quote     = lit('\'');
  oparen    = space + lit('(') + space;
  cparen    = space + lit(')') + space;
  obrace    = space + lit('{') + space;
  cbrace    = space + lit('}') + space;
  osqbrack  = space + lit('[') + space;
  csqbrack  = space + lit(']') + space;

  sign = lit('+') | lit('-');

  sym  = isymchar + ~symchars;
  num  = ~sign + digits;
  flt  = ~sign + digits + ~(lit('.') + digits);
  str  = dquote   + strchars + dquote;
}

// Tokens and stuff  (mostly terminals)
Lexer lex;

// Syntax (non-terminals)
NonTerminal<ATOM>  atom; 
NonTerminal<LIST>  list; 
NonTerminal<HEAD>  head; 
NonTerminal<TAIL>  tail; 
NonTerminal<ITEM>  item; 
NonTerminal<SEXPR> sexpr; 
NonTerminal<PROG>  program;

void load(std::string path) {
  std::ifstream t(path);
  std::stringstream buffer;
  buffer << t.rdbuf();
  std::string prelude = buffer.str(); 

  std::string input = prelude;
  auto b = input.begin();
  auto e = input.end();

  try {
    while (b < e) { 
      ParseNode* pRoot  = program.parse(b, e);
      //pRoot->print();
      Expr* pExprRoot = buildExpr(pRoot);
      pExprRoot = pExprRoot->eval(pGlobalScope.get());
      pExprRoot->print();
      delete pRoot;
      //delete pExprRoot;
      std::cout << std::endl;
    }
  }
  catch (std::exception& e) {
    std::cout << e.what() << std::endl;
  }
}

int main () {
  try {
    atom    = lex.space + (lex.flt | lex.num | lex.str | lex.sym);
    item    = !sexpr;

    head    = lex.oparen;
    tail    = lex.cparen | (item + !tail);
    list    = lex.space + (head + tail); 

    sexpr   = atom | list;   
    program = lex.space + sexpr;

    initGlobalScope();

    std::string input = "(load \"prelude.cvd\")";
    auto b = input.begin();
    auto e = input.end();
    while (!std::cin.eof()) {
      if (b < e) {
        try {
          ParseNode* pRoot  = program.parse(b, e);
          //pRoot->print();
          Expr* pExprRoot = buildExpr(pRoot);
          pExprRoot = pExprRoot->eval(pGlobalScope.get());
          pExprRoot->print();
          delete pRoot;
          //delete pExprRoot;
        }
        catch (std::exception& e) {
          std::cout << e.what() << std::endl;
        }
      }

      std::cout << std::endl << ">>> ";
      std::getline(std::cin, input);
      b = input.begin();
      e = input.end();
    }
  } 
  catch (std::exception& e) {
    std::cout << e.what() << std::endl;
  }
  
  return 0;
}

//------------------------------------------------------------------------------
