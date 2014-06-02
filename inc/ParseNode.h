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

typedef std::string::iterator StrIt;

//------------------------------------------------------------------------------
/* 
 *  ParseNode represents a node in the resulting Abstract Syntax Tree.  
 *
 *  Currently, ASTs are binary trees (With Left and Right subtrees)
 */
struct ParseNode {
  enum Position {
    UNKNOWN,
    ROOT,
    LEFT,
    RIGHT,
  };

  static const int noContext = -1;

  // Construct an empty ParseNode
  ParseNode(const std::string& m = "", ParseNode* pL = 0, ParseNode* pR = 0) 
  : match(m)
  , position(UNKNOWN)
  , indirection(0) 
  , pLeft(pL)
  , pRight(pR)
  {}

  ParseNode(ParseNode& other) 
  : match(other.match)
  , position(other.position)
  , indirection(other.indirection) 
  , pLeft(other.pLeft.release())
  , pRight(other.pRight.release())
  {}
    
  void terminate() {
    pLeft.reset(0);
    pRight.reset(0);
  }
  
  void print(unsigned depth = 0, Position position = ROOT) {
    std::string pos;
    switch (position) {
      case LEFT:   pos = "L"; break;
      case RIGHT:  pos = "R"; break;
      case ROOT:   pos = "^"; break;
      case UNKNOWN: 
      default:     pos = "E"; break;
    }

    std::cout << pos 
              << std::setfill(' ')   << std::setw(3) 
              << depth << ": ";

    std::cout << std::setfill(' ')   << std::setw(4) 
              << name << "["      << indirection << "]" 
              << std::string(4, ' ') << std::string(2*depth, '-') 
              << " "                 << match << std::endl;

    if (pLeft.get())  pLeft->print(depth+1, LEFT);
    if (pRight.get()) pRight->print(depth+1, RIGHT);
  }

  ParseNode* getLeft() const {
    if (pLeft.get()) {
      return pLeft.get(); 
    }
  
    throw std::runtime_error("ParseNode has no Left");
    return 0;
  }

  ParseNode* getRight() const {
    if (pRight.get()) {
      return pRight.get();
    }

    throw std::runtime_error("ParseNode has no Right");
    return 0;
  }

private:

public:
  std::string                match;
  std::string                name;

  int                        position;
  int                        indirection;

  std::unique_ptr<ParseNode> pLeft;
  std::unique_ptr<ParseNode> pRight;

  std::list<std::unique_ptr<ParseNode>> children;
};

//------------------------------------------------------------------------------

// A Parselet is anything that takes an iterable and turns it into into an AST node
template <typename N = ParseNode, typename T = std::string>
struct Parselet {
  typedef typename T::iterator                                      Iterator;
  typedef N                                                         NodeType;
  typedef std::function<NodeType* (Iterator& begin, Iterator& end)> Parser;

  // Execute the parser and decorate the node with some context
  virtual NodeType* parse(Iterator& first, Iterator& last) const = 0;
};

// A Parser is any function that takes string iterators and returns a ParseNode
// Errors are handled by throwing an exception
typedef std::function<ParseNode* (StrIt& begin, StrIt& end)> Parser;

// What are rules?
struct Rule : Parselet<ParseNode> {

  // The nil Parser is useful for initializing nil rules

  static ParseNode* nil(StrIt& begin, StrIt& end) {
    return 0;
  }

  Rule (bool terminal = false) 
  : parse_  (nil) 
  , terminal_  (false) 
  , name_ (""){
  }

  Rule (const Parser& p) 
  : parse_  (p) 
  , terminal_  (false) 
  , name_ (""){
  }

  Rule (const Rule& r, const char* as) 
  : parse_     (r.parse_) 
  , terminal_  (r.terminal_)
  , name_      (as) {
  }

  // Execute the parser and decorate the node with some context
  ParseNode* parse(StrIt& first, StrIt& last) const {
    ParseNode* pNode = parse_(first, last);
    if (pNode) { 
      pNode->name    = name_;

      if (terminal_) {
        pNode->terminate();
      }
    }
    return pNode;
  }

  // Return a new rule with a name
  Rule as(const char* name) {
    return Rule(*this, name);
  }

  Parser  parse_;
  bool    terminal_;

  const char*  name_;
};

void error(const std::string& match, const std::string& context) {
    std::stringstream errorMsg;
    errorMsg << "= fail in \'" << context << "\': " << match;
    throw std::runtime_error(errorMsg.str());
}

//------------------------------------------------------------------------------
// Primitives 
static Rule 
range(char a, char b) {
  return Rule([=](StrIt& first, StrIt& last) -> ParseNode* {
    auto match = std::string(first, std::next(first));
    if (*first >= a && *first <= b) {
      std::advance(first, 1);
    } else {
      error(match, "range");
    }
    return new ParseNode(match);
  });
};

static Rule 
lit(char c) {
  return Rule([=](StrIt& first, StrIt& last) -> ParseNode* {
    auto match = std::string(first, std::next(first));
    if (*first == c) {
      std::advance(first, 1);
    } else {
      error(match, "lit");
    }
    return new ParseNode(match);
  });
};

static Rule 
lit(const std::string& str) {
  return Rule([=](StrIt& first, StrIt& last) -> ParseNode* {
    auto match = std::string(first, std::next(first, str.length()));
    if (match == str) {
      std::advance(first, str.length());
    } else {
      error(match, "lit");
    }
    return new ParseNode(match);
  });
};

static Rule 
any(const std::string& str) {
  return Rule([=](StrIt& first, StrIt& last) -> ParseNode* {
    auto match = std::string(first, std::next(first));
    for (auto c : str) {
      if (*first == c) {
        std::advance(first, 1);
        return new ParseNode(match);
      }
    }

    error(match, "any");
    return 0;
  });
};

static Rule 
skip(char c, char d) {
  return Rule([=](StrIt& begin, StrIt& end) -> ParseNode* {
    while ((*begin == c || *begin == d) && begin != end) {
      begin++;
    }
    return 0;
  });
}

#include "ParseCombinators.h"

//------------------------------------------------------------------------------
