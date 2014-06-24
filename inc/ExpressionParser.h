#include "Lexer.h"

typedef StringLexer::Token Token;

struct ExpressionNode;
struct ExpressionParser;

struct ExpressionParselet {
  ExpressionParselet(int precedence = 0) : precedence_(precedence) {}

  virtual ExpressionNode* prefix(ExpressionParser* pParser) {
    //throw std::runtime_error("syntax error - no prefix");
    return 0;
  }

  virtual ExpressionNode* infix(ExpressionParser* pParser, ExpressionNode* pLeft) {
    //throw std::runtime_error("syntax error - no infix");
    return 0;
  }

  int precedence() const {   
    return precedence_; 
  }

  int precedence_;
};

struct ExpressionNode {
  ExpressionNode(Token token, ExpressionNode* pLeft = 0, ExpressionNode* pRight = 0) 
  : token_(token) 
  , pLeft_(pLeft) 
  , pRight_(pRight) {}

  void print(unsigned depth = 0) {
    std::string pos;
    std::cout << pos 
              << std::setfill(' ')   << std::setw(3) 
              << depth << ": ";

    std::cout << std::setfill(' ')   << std::setw(15) 
              << token_.type 
              << std::string(4, ' ') << std::string(2*depth, '-') 
              << " "                 << '\'' << lexeme() << '\'' <<  std::endl;

    if (pLeft_.get())  pLeft_->print(depth+1);
    if (pRight_.get()) pRight_->print(depth+1);
  }

  template <typename V>
  void visit(V v) {
    v(this);
    if (pLeft_.get())  pLeft_->visit(v);
    if (pRight_.get()) pRight_->visit(v);
  }

  const std::string& type()  const { return token_.type; }
  const std::string& lexeme() const { return token_.lexeme; }

  ExpressionNode* left()  { return pLeft_.get(); }
  ExpressionNode* right() { return pRight_.get(); }

  Token                           token_;
  std::unique_ptr<ExpressionNode> pLeft_;
  std::unique_ptr<ExpressionNode> pRight_;
};

// AST node representing "" (Nothing, nil, null zilch)
struct NilNode : public ExpressionNode { 
  NilNode(Token token = Token("", ""))
  : ExpressionNode(token) {}
};

struct ExpressionParser { 
  ExpressionParser(StringLexer& lex) 
    : lex_  (lex) { }

  ExpressionNode* parse(int precedence = 0) {
    auto token          = lex_.nextToken();
    auto prefixParselet = prefixOps_.find(token.type);
    if (prefixParselet == prefixOps_.end()) {
      //throw std::runtime_error("syntax error");
    }
    auto pLeft          = prefixParselet->second->prefix(this);     

    while (precedence < getInfixPrecedence()) { 
      token              = lex_.nextToken();
      auto infixParselet = infixOps_.find(token.type);
      if (infixParselet == infixOps_.end()) {
        //throw std::runtime_error("syntax error");
      }
      pLeft              = infixParselet->second->infix(this, pLeft);     
    }
    return pLeft;
  }
   
  int getInfixPrecedence() {
    auto infixParselet = infixOps_.find(lex_.lookahead().type);
    if (infixParselet == infixOps_.end()) return 0;
    return infixParselet->second->precedence();
  }

  void usePrefix(ExpressionParselet* pParselet, std::string token) {
    prefixOps_[token] = pParselet;
  }

  void useInfix(ExpressionParselet* pParselet, std::string token) {
    infixOps_[token] = pParselet;
  }

  StringLexer& getLexer() { return lex_; }

  StringLexer&                               lex_;
  std::map<std::string, ExpressionParselet*> prefixOps_;
  std::map<std::string, ExpressionParselet*> infixOps_;
};
