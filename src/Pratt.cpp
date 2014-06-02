#include "ParseNode.h"

#include <regex>

// A Lexer transforms some stream into another stream of tokens
template <typename S, typename T>
struct Lexer {
  typedef typename S::iterator              StreamIterator;
  typedef typename std::vector<T>::iterator TokenIterator;

  virtual TokenIterator currentToken() const = 0;
  virtual void          advance()            = 0;
};


template <typename N, typename M = N, typename T = std::string>
struct InfixParselet {
  typedef typename T::iterator                                      Iterator;
  typedef N                                                         NodeType;
  typedef N                                                         LeftType;
  typedef std::function<NodeType* (Iterator& begin, Iterator& end)> Parser;

  virtual NodeType* parse(LeftType* pLeft) const = 0;
};

struct ExpressionNode {
  ExpressionNode(const std::string& match, const std::string& token, int lbp = 0) 
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

  void print(unsigned depth = 0) {
    std::string pos;
    std::cout << pos 
              << std::setfill(' ')   << std::setw(3) 
              << depth << ": ";

    std::cout << std::setfill(' ')   << std::setw(15) 
              << token_ 
              << std::string(4, ' ') << std::string(2*depth, '-') 
              << " "                 << '\'' << match_ << '\'' <<  std::endl;

    if (pLeft.get())  pLeft->print(depth+1);
    if (pRight.get()) pRight->print(depth+1);
  }
  
  int lbp() const { return lbp_; }  //XXX

  int lbp_;
  std::string token_;
  std::string match_;
  std::unique_ptr<ExpressionNode> pLeft;
  std::unique_ptr<ExpressionNode> pRight;
};

struct NilNode : public ExpressionNode { 
  NilNode(const std::string& match = "", const std::string& token = "", int rbp = 0)
  : ExpressionNode(match, token, rbp) {}
};

struct Tokenizer {
  struct Token {
    Token(const std::string& id, Rule rule, int lbp) : id_(id), rule_(rule), lbp_(lbp) {}

    std::string id_;
    Rule        rule_;
    int         lbp_; 
  };

  static ExpressionNode* next() {
    for (auto t : tokens_) {
      auto v = std::get<1>(t);
      ParseNode* m = 0; 

      auto original = begin;
      try {
        m = std::get<0>(v).parse(begin, end);
      } catch (std::exception& e) {
        //std::cout << "backtrack via exception = bad" << std::endl;
        begin = original;
      }

      if (m) {
        //return new ExpressionNode(m->match, std::get<1>(v));
        return std::get<2>(v)(m->match, std::get<0>(t));
      }
    }  

    std::cout << "no token to consume" << std::endl;
    return new NilNode();
  }
  
  template <typename NodeType>
  static void addToken(std::string name, Rule token, int rbp) {
    tokens_.insert(std::make_tuple(name, std::make_tuple(token, rbp, 
      [=](const std::string& match, const std::string& token) -> ExpressionNode* {
        return new NodeType(match, token, rbp);
      }
    )));
  }

  static std::map<
    std::string,  
    std::tuple<
      Rule, int, 
      std::function<ExpressionNode* (const std::string& match, const std::string& token)>
    >
  > tokens_;
  static ExpressionNode* token;
  static std::string::iterator begin;
  static std::string::iterator end;
};

std::map<std::string, std::tuple<Rule, int, std::function<ExpressionNode* (const std::string&, const std::string&)>>> Tokenizer::tokens_;
ExpressionNode* Tokenizer::token;
std::string::iterator Tokenizer::begin;
std::string::iterator Tokenizer::end;

struct Expression { // This should be a Parselet<>  
  Expression(int rbp = 0) : rbp_(rbp) { }

  // Execute the parser and decorate the node with some context
  ExpressionNode* parse() {
    std::cout << "expr rbp = " << rbp_ << "\n";

    auto pToken      = Tokenizer::token;     
    Tokenizer::token = nextToken();          
    auto pLeft       = pToken->prefix();     
    while (rbp_ < Tokenizer::token->lbp()) { 
      pToken           = Tokenizer::token;    
      Tokenizer::token = nextToken();         
      pLeft            = pToken->infix(pLeft);
    }
  
    return pLeft;
  }

  ExpressionNode* nextToken() {
    return Tokenizer::next();
  }
  
  template <typename NodeType>
  void addToken(std::string name, Rule token, int rbp) {
    Tokenizer::addToken<NodeType>(name, token, rbp);
  }

  int rbp_;
};

struct LiteralParselet : public Parselet<ExpressionNode> {
  LiteralParselet(Rule rule) : rule_ (rule) {}
  ExpressionNode* parse(Iterator& first, Iterator& last) const {
    auto pNode = rule_.parse(first, last); // get token
    return new ExpressionNode(pNode->match, "LITERAL", 0);
  }
private:
  Rule rule_;
};

struct PrefixParselet : public Parselet<ExpressionNode> {
  PrefixParselet(Rule rule) : rule_ (rule) {}
  ExpressionNode* parse(Iterator& first, Iterator& last) const {
    auto pNode = rule_.parse(first, last); // get token

    auto pExpressionNode = new ExpressionNode(pNode->match, "PREFIX_OP", 100);
    pExpressionNode->pRight.reset(Expression(100).parse());
    return pExpressionNode;
  }
private:
  Rule rule_;
};


struct LiteralNode : public ExpressionNode {
  LiteralNode(const std::string& match, const std::string& token, int rbp) 
  : ExpressionNode(match, token, rbp) {}
  ExpressionNode* prefix() {
    std::cout << "literal nud = " << match_ << "\n";
    return this; 
  }
};

struct Prefix : public ExpressionNode {
  Prefix(const std::string& match, const std::string& token, int rbp) 
  : ExpressionNode(match, token, rbp) {}
  ExpressionNode* prefix() {
    std::cout << "unary nud = " << match_ << "\n";
    this->pRight.reset(Expression(lbp()).parse());
    return this; 
  }
};

struct InfixLeft : public ExpressionNode {
  InfixLeft(const std::string& match, const std::string& token, int rbp) 
  : ExpressionNode(match, token, rbp) {}
  ExpressionNode* infix(ExpressionNode* pLeft) {
    std::cout << "mul led = " << match_ << "\n";
    this->pRight.reset(Expression(lbp()).parse());
    this->pLeft.reset(pLeft);
    //pRight = Expression(20);
    // semantic action?
    return this;
  }
};

struct InfixRight : public ExpressionNode {
  InfixRight(const std::string& match, const std::string& token, int rbp) 
  : ExpressionNode(match, token, rbp) {}
  ExpressionNode* infix(ExpressionNode* pLeft) {
    std::cout << "mul led = " << match_ << "\n";
    this->pRight.reset(Expression(lbp()-1).parse());
    this->pLeft.reset(pLeft);
    //pRight = Expression(20);
    // semantic action?
    return this;
  }
};

int main () {

  /*
  Rule space = skip(' ', '\n');
  Rule obrack = lit('[');
  Rule cbrack = lit(']');
  Rule atom   = (space + range('a', 'z') + space); 
  Rule list, list_lit, expr;
  expr     = atom.as("atom");
  list     = expr.as("head") + (~list).as("tail");
  list_lit = obrack + list.as("list") + cbrack;

  auto test = std::string("[a b c d e f g]");
  Rule program = list_lit;
  */

  //Infix op_add(lit('+'), 10)
  //Infix op_mul(lit('*'), 20)
  Tokenizer::addToken<NilNode>       ("nil",      "",               0);
  Tokenizer::addToken<LiteralNode>   ("#",        range('0', '9'),  0);
  Tokenizer::addToken<InfixLeft>     ("+",        lit('+'),        10);
  Tokenizer::addToken<Prefix>        ("unary -",  lit('-'),       100);
  // TODO fix this shit Tokenizer::addToken<InfixRight>  ("binary -", lit('-'),        10);
  Tokenizer::addToken<InfixRight>    ("binary **", lit("**"),        30);
  //Tokenizer::addToken<InfixLeft>  ("binary *", lit('*'),        20);
  // TODO Split infix/prefix parsers up

  Expression expr(0);
  //Rule *program = Expression::ruleAdaptor(&expr);

  auto test = std::string("1**4**3+4**3+-5**4");
  auto b = test.begin();
  auto e = test.end();
  Tokenizer::begin = b;
  Tokenizer::end   = e;
  Tokenizer::token = Tokenizer::next();  // initialize w/ first token

  if (b < e) {
    try {
      ExpressionNode* pRoot  = expr.parse();
      pRoot->print();
      delete pRoot;
    }
    catch (std::exception& e) {
      std::cout << e.what() << std::endl;
    }
  }
}
