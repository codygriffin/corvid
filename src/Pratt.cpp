#include "ParseNode.h"
#include "Lexer.h"

// This isn't a Parselet because it needs a left side 
template <typename N, typename M = N, typename T = std::string>
struct InfixParselet {
  typedef typename T::iterator                                      Iterator;
  typedef N                                                         NodeType;
  typedef N                                                         LeftType;
  typedef std::function<NodeType* (Iterator& begin, Iterator& end)> Parser;

  virtual NodeType* parse(LeftType* pLeft) const = 0;
};

struct ExpressionNode;
struct Token;
typedef std::function<
  ExpressionNode* (const std::string& match, Token token)
> TokenNodeGenerator;

struct Token {
  Token(Rule r, int l, TokenNodeGenerator f) : value(""), rule(r), lbp(l), gen(f) {}

  std::string        value;
  Rule               rule;
  int                lbp; 
  TokenNodeGenerator gen;
};

// This really ought to be a ParseNode
struct ExpressionNode {
  ExpressionNode(const std::string& match, Token token, int lbp = 0) 
  : lbp_(lbp)
  , token_(token) 
  , match_ (match) {}

  virtual ExpressionNode* prefix() {
    std::cout << ">> no prefix" << std::endl;
    //throw std::runtime_error("syntax error - no prefix");
    return 0;
  }

  virtual ExpressionNode* infix(ExpressionNode* pLeft) {
    std::cout << ">> no infix" << std::endl;
    //throw std::runtime_error("syntax error - no infix");
    return 0;
  }

  virtual bool infix() { return false; }

  void print(unsigned depth = 0) {
    std::string pos;
    std::cout << pos 
              << std::setfill(' ')   << std::setw(3) 
              << depth << ": ";

    std::cout << std::setfill(' ')   << std::setw(15) 
              << token_.value 
              << std::string(4, ' ') << std::string(2*depth, '-') 
              << " "                 << '\'' << match_ << '\'' <<  std::endl;

    if (pLeft.get())  pLeft->print(depth+1);
    if (pRight.get()) pRight->print(depth+1);
  }
  
  int lbp() const { return lbp_; }  //XXX
  const std::string& match() const { return match_; }

  int lbp_;
  Token token_;
  std::string match_;
  std::unique_ptr<ExpressionNode> pLeft;
  std::unique_ptr<ExpressionNode> pRight;
};

// AST node representing "" (Nothing, nil, null zilch)
struct NilNode : public ExpressionNode { 
  NilNode(const std::string& match = "wha?", Token token = Token(Rule(), 0, nullptr), int rbp = 0)
  : ExpressionNode(match, token, rbp) {}
};

struct Tokenizer {
  template <typename F>
  static ExpressionNode* next(F f) {
    static std::vector<ExpressionNode*> pNodeHeap; 
    pNodeHeap.clear();

    std::cout << "Next token" << std::endl;
    auto original = begin;

    for (auto token : tokens_) {
      std::cout << "\t tokenizing string: " << std::string(begin, end) << std::endl;
      ParseNode* pNode = 0; 
      try {
        pNode = token.rule.parse(begin, end);
      } catch (std::exception& e) {
      }

      if (pNode) {
        //return new ExpressionNode(m->match, std::get<1>(v));
        auto pNewExpressionNode = token.gen(pNode->match, token);
        std::cout << "\t candidate token: " << pNewExpressionNode->match() << std::endl;
        pNodeHeap.push_back(pNewExpressionNode);
        // TODO move to operator < on Token
        std::push_heap(pNodeHeap.begin(), pNodeHeap.end(), f);
      }

      // rewind for next try
      begin = original;
    }

    if (!pNodeHeap.empty()) {
      std::pop_heap(pNodeHeap.begin(), pNodeHeap.end());
      auto pMatchedNode = pNodeHeap.back();
      pNodeHeap.pop_back();
      for (auto e : pNodeHeap) { delete e; }
      begin = original + pMatchedNode->match().length();
      std::cout << "\t matched token: " << pMatchedNode->match() << std::endl;
      return pMatchedNode;
    }
    else {
      std::cout << "\t no candidate tokens" << std::endl;
      return new NilNode();
    }
  }

  template <typename NodeType>
  static void useToken(Rule token, int rbp) {
    auto gen = [=](const std::string& match, Token token) -> ExpressionNode* {
      token.value = match;
      return new NodeType(match, token, rbp);
    };

    tokens_.push_back(Token(token, rbp, gen));
  }

  static void consume(const std::string& t) {
    if (token->match() == t) {
      token = next(
        [=](ExpressionNode* pA, ExpressionNode* pB) {
          if (!pA->infix() && pB->infix()) return true;
          return pA->match().length() < pB->match().length();
        });
      std::cout << "\t after consuming " << t << ": " << std::string(begin, end) << std::endl;
    }
    //throw std::runtime_error("error consuming token");
  }


  static std::vector<Token> tokens_;
  static ExpressionNode* token;
  static std::string::iterator begin;
  static std::string::iterator end;
};

std::vector<Token> Tokenizer::tokens_;
ExpressionNode* Tokenizer::token;
std::string::iterator Tokenizer::begin;
std::string::iterator Tokenizer::end;

struct Expression { // This should be a Parselet<>  
  Expression(int rbp = 0) : rbp_(rbp) { }

  // Execute the parser and decorate the node with some context
  // TODO tokenizer state is inside...
  ExpressionNode* parse() {
    std::cout << "parsing expression\n";
    auto pToken      = Tokenizer::token;     
    std::cout << "token = " << pToken->match() << "\n";
    Tokenizer::token = nextPrefixToken();          
    std::cout << "next = " << Tokenizer::token->match() << "\n";
    auto pLeft       = pToken->prefix();     
    while (rbp_ < Tokenizer::token->lbp()) { 
      pToken           = Tokenizer::token;    
      Tokenizer::token = nextInfixToken();         
      pLeft            = pToken->infix(pLeft);
    }
  
    return pLeft;
  }

  ExpressionNode* nextPrefixToken() {
    return Tokenizer::next(
      [=](ExpressionNode* pA, ExpressionNode* pB) {
        if (!pA->infix() && pB->infix()) return true;
        return pA->match().length() < pB->match().length();
      });
  }

  ExpressionNode* nextInfixToken() {
    return Tokenizer::next(
      [=](ExpressionNode* pA, ExpressionNode* pB) {
        if (pA->infix() && !pB->infix()) return true;
        return pA->match().length() < pB->match().length();
      });
  }

  template <typename NodeType>
  void useToken(std::string name, Rule token, int rbp) {
    Tokenizer::useToken<NodeType>(name, token, rbp);
  }

  int rbp_;
};

//struct PrefixParselet : public Parselet<ExpressionNode> {
//  PrefixParselet(Rule rule) : rule_ (rule) {}
//  ExpressionNode* parse(Iterator& first, Iterator& last) const {
//    auto pNode = rule_.parse(first, last); // get token
//
//    auto pExpressionNode = new ExpressionNode(pNode->match, "PREFIX_OP", 100);
//    pExpressionNode->pRight.reset(Expression(100).parse());
//    return pExpressionNode;
//  }
//private:
//  Rule rule_;
//};


struct LiteralNode : public ExpressionNode {
  LiteralNode(const std::string& match, Token token, int rbp) 
  : ExpressionNode(match, token, rbp) {}
  ExpressionNode* prefix() {
    std::cout << "LITERAL = " << match_ << "\n";
    return this; 
  }
};

//struct LiteralParselet : public Parselet<ExpressionNode> {
//  LiteralParselet(Rule rule) : rule_ (rule) {}
//  ExpressionNode* parse(Iterator& first, Iterator& last) const {
//    auto pNode = rule_.parse(first, last); // get token
//    return new ExpressionNode(pNode->match, "LITERAL", 0);
//  }
//private:
//  Rule rule_;
//};


struct Prefix : public ExpressionNode {
  Prefix(const std::string& match, Token token, int rbp) 
  : ExpressionNode(match, token, rbp) {}
  ExpressionNode* prefix() {
    std::cout << "PREFIX = " << match_ << "\n";
    this->pRight.reset(Expression(lbp()).parse());
    return this; 
  }
};

struct InfixLeft : public ExpressionNode {
  InfixLeft(const std::string& match, Token token, int rbp) 
  : ExpressionNode(match, token, rbp) {}
  ExpressionNode* infix(ExpressionNode* pLeft) {
    std::cout << "INFIX = " << match_ << "\n";
    std::cout << " parsing rhs\n";
    this->pRight.reset(Expression(lbp()).parse());
    std::cout << " result rhs: " << pRight->match() << "\n";
    this->pLeft.reset(pLeft);
    std::cout << " lhs: " << pLeft->match() << "\n";
    return this;
  }
  bool infix() { return true; }
};

struct InfixRight : public ExpressionNode {
  InfixRight(const std::string& match, Token token, int rbp) 
  : ExpressionNode(match, token, rbp) {}
  ExpressionNode* infix(ExpressionNode* pLeft) {
    std::cout << "INFIXR = " << match_ << "\n";
    this->pRight.reset(Expression(lbp()-1).parse());
    std::cout << " result rhs: " << pRight->match() << "\n";
    this->pLeft.reset(pLeft);
    return this;
  }
  bool infix() { return true; }
};

struct GroupLeft : public ExpressionNode {
  GroupLeft(const std::string& match, Token token, int rbp) 
  : ExpressionNode(match, token, rbp) {}
  ExpressionNode* prefix() {
    std::cout << "GROUP = " << match_ << "\n";
    std::cout << " parsing rhs\n";
    this->pRight.reset(Expression(0).parse());
    std::cout << " result rhs: " << pRight->match() << "\n";
    std::cout << "closing w/ )\n"; 
    Tokenizer::consume(")");
    return this;
  }
};


struct ApplyLeft : public ExpressionNode {
  ApplyLeft(const std::string& match, Token token, int rbp) 
  : ExpressionNode(match, token, rbp) {}
  ExpressionNode* infix(ExpressionNode* pLeft) {
    std::cout << "INFIX = " << match_ << "\n";
    std::cout << " parsing rhs\n";
    this->pRight.reset(Expression(0).parse());
    std::cout << " result rhs: " << pRight->match() << "\n";
    std::cout << "closing w/ )\n"; 
    Tokenizer::consume(")");
    this->pLeft.reset(pLeft);
    return this;
  }
  bool infix() { return true; }
};


int main () {
  Rule lalpha, lalphas, digit, digits, skip;
  skip      = maybe(lit(' ') 
                  | lit('\t') 
                  | lit('\n'));

  auto _ = [&](Rule rule) { return skip + rule + skip; };

  lalpha    = range('a', 'z');
  digit     = range('0', '9');
  lalphas   = seq(lalpha, maybe(lazy(lalphas)));
  digits    = seq(digit,  maybe(lazy(digits)));

  Tokenizer::useToken<NilNode>       (_(Rule()),           0);
  Tokenizer::useToken<NilNode>       (_(lit(")")),         0);
  Tokenizer::useToken<LiteralNode>   (_(lalphas | digits), 0);
  Tokenizer::useToken<GroupLeft>     (_(lit("(")),         0);
  Tokenizer::useToken<InfixLeft>     (_(lit("+")),        10);
  Tokenizer::useToken<InfixLeft>     (_(lit("-")),        10);
  Tokenizer::useToken<InfixLeft>     (_(lit("*")),        20);
  Tokenizer::useToken<InfixRight>    (_(lit("**")),       30);
  Tokenizer::useToken<Prefix>        (_(lit("-")),       100);
  Tokenizer::useToken<InfixLeft>     (_(lit(".")),       200);
  Tokenizer::useToken<ApplyLeft>     (_(lit("(")),       500);
 
  Expression expr(0);

  //auto test = std::string("(12 * (c + -b)).add(122)");
  auto test = std::string("fn(1+1)+(4*3)");
  auto b = test.begin();
  auto e = test.end();
  Tokenizer::begin = b;
  Tokenizer::end   = e;
  Tokenizer::token = Tokenizer::next(
    [=](ExpressionNode* pA, ExpressionNode* pB) {
      if (pA->infix() && !pB->infix()) return true;
      return pA->match().length() < pB->match().length();
    });

  if (b < e) {
    try {
      ExpressionNode* pRoot = expr.parse();
      if (pRoot) {
        pRoot->print();
        delete pRoot;
      }
    }
    catch (std::exception& e) {
      std::cout << e.what() << std::endl;
    }
  }
}
