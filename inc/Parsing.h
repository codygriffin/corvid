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
  , context(noContext)
  , position(UNKNOWN)
  , indirection(0) 
  , pLeft(pL)
  , pRight(pR)
  {}
    
  void terminate() {
    pLeft.reset(0);
    pRight.reset(0);
  }
  
  bool hasContext() {
    return context != noContext;
  }

  void fillContext() {
    if (hasContext()) {
      fillContext(pLeft.get());
      fillContext(pRight.get());
    }
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
              << context << "["      << indirection << "]" 
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
  void fillContext(ParseNode* pNode) {
    if (pNode && !pNode->hasContext()) {
      pNode->context = context;
      pNode->indirection  = indirection + 1;
      pNode->fillContext();
    }
  }

public:
  std::string                match;
  int                        context;
  int                        position;
  int                        indirection;

  std::unique_ptr<ParseNode> pLeft;
  std::unique_ptr<ParseNode> pRight;
};

// A Parser is any function that takes string iterators and returns a ParseNode
// Errors are handled by throwing an exception
typedef std::function<ParseNode* (StrIt& begin, StrIt& end)> Parser;

// What are rules?
struct Rule {

  // The nil Parser is useful for initializing nil rules
  static ParseNode* nil(StrIt& begin, StrIt& end) {
    return 0;
  }

  Rule (int context = ParseNode::noContext, bool terminal = false) 
  : parse_  (nil) 
  , context_   (-1) 
  , terminal_  (false) {
  }

  Rule (const Parser& p) 
  : parse_  (p) 
  , context_   (-1) 
  , terminal_  (false) {
  }

  Rule (const Parser&& p) 
  : parse_  (p) 
  , context_   (-1) 
  , terminal_  (false) {
  }

  ParseNode* parse(StrIt& first, StrIt& last) const {
    ParseNode* pNode = parse_(first, last);
    if (pNode) { 
      pNode->context = context_;
      pNode->fillContext();
      if (terminal_) {
        pNode->terminate();
      }
    }
    return pNode;
  }

  ParseNode* parse(const char* pText) const {
    std::string temp(pText);
    auto first = temp.begin();
    auto last  = temp.end();
    ParseNode* pNode = parse_(first, last);
    if (pNode) { 
      pNode->context = context_;
      pNode->fillContext();
      if (terminal_) {
        pNode->terminate();
      }
    }
    return pNode;
  }

  Parser  parse_;
  int     context_;
  bool    terminal_;
};

// NonTerminal rules 
template <int Tag>
struct NonTerminal : public Rule {
  NonTerminal()                                      {Rule::context_ = Tag;}
  NonTerminal(const Rule& rule) : Rule (rule.parse_) {Rule::context_ = Tag;}
};

// Terminal rules 
template <int Tag>
struct Terminal : public Rule {
  Terminal()                                      {Rule::context_ = Tag; Rule::terminal_ = true;}
  Terminal(const Rule& rule) : Rule (rule.parse_) {Rule::context_ = Tag; Rule::terminal_ = true;}
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
      error(match, "parse");
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
      error(match, "parse");
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
      error(match, "parse");
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

    error(match, "parse");
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

//------------------------------------------------------------------------------
// Combinators
Rule seq(const Rule& a, const Rule& b) {
  Rule seqRule;
  seqRule.parse_ = [=](StrIt& first, StrIt& last) -> ParseNode* {
    auto original = first;
    try {
      ParseNode* pNode = new ParseNode(); 
      pNode->pLeft.reset (a.parse(first, last));
      pNode->pRight.reset(b.parse(first, last));
      pNode->match = std::string(original, first);
      return pNode;
    }
    catch (std::exception& e) {
      first = original;
      throw;
    }
  };
  return seqRule;
}

Rule alt(const Rule& a, const Rule& b) {
  Rule altRule;
  altRule.parse_ = [=](StrIt& first, StrIt& last) -> ParseNode* {
    auto original = first;
    try {
      ParseNode* pNode = new ParseNode(); 
      pNode->pLeft.reset(a.parse(first, last));
      pNode->match = std::string(original, first);
      return pNode;
    }
    catch (std::exception& e) {
      first = original;
      ParseNode* pNode = new ParseNode(); 
      pNode->pRight.reset(b.parse(first, last));
      pNode->match = std::string(original, first);
      return pNode;
    }
  };
  return altRule;
}

// Add a layer of indirection via a lambda
Rule lazy(const Rule& a) {
  const Rule* pA = &a;
  return Rule([=](StrIt& begin, StrIt& end) -> ParseNode* {
    return pA->parse(begin, end);
  });
}

// maybe
Rule maybe(const Rule& a) {
  return alt(a, Rule());
}

//------------------------------------------------------------------------------
// operators
Rule operator+ (const Rule& a, const Rule& b) {
  return seq(a, b);
}

Rule operator& (const Rule& a, const Rule& b) {
  return seq(a, b);
}

Rule operator| (const Rule& a, const Rule& b) {
  return alt(a, b);
}

Rule operator! (const Rule& a) {
  return lazy(a);
}

Rule operator~ (const Rule& a) {
  return maybe(a);
}

Rule operator* (const Rule& a) {
  return maybe(lazy(a));
}

//------------------------------------------------------------------------------
