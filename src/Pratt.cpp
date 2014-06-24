#include "ParseNode.h"
#include "Lexer.h"

struct ExpressionNode;
struct Token;
typedef std::function<
  ExpressionNode* (const std::string& match, Token token)
> TokenNodeGenerator;

// This should be in Lexer
struct Token {
  Token() {} 
  Token(std::string i, std::string v) 
    : id(i)
    , value(v) {
  }

  std::string        value;
  std::string        id;
};

struct ExpressionNode;
struct ExpressionParser;

struct ExpressionParselet {
  ExpressionParselet(int precedence = 0) : precedence_(precedence) {}

  virtual ExpressionNode* prefix(ExpressionParser* pParser) {
    throw std::runtime_error("syntax error - no prefix");
    return 0;
  }

  virtual ExpressionNode* infix(ExpressionParser* pParser, ExpressionNode* pLeft) {
    throw std::runtime_error("syntax error - no infix");
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
              << token_.id 
              << std::string(4, ' ') << std::string(2*depth, '-') 
              << " "                 << '\'' << match() << '\'' <<  std::endl;

    if (pLeft_.get())  pLeft_->print(depth+1);
    if (pRight_.get()) pRight_->print(depth+1);
  }

  const std::string& match() const { return token_.value; }

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
    auto prefixParselet = prefixOps_.find(token.first);
    if (prefixParselet == prefixOps_.end()) {
      throw std::runtime_error("syntax error");
    }
    auto pLeft          = prefixParselet->second->prefix(this);     

    while (precedence < getInfixPrecedence()) { 
      token              = lex_.nextToken();
      auto infixParselet = infixOps_.find(token.first);
      if (infixParselet == infixOps_.end()) {
        throw std::runtime_error("syntax error");
      }
      pLeft              = infixParselet->second->infix(this, pLeft);     
    }
    return pLeft;
  }
   
  int getInfixPrecedence() {
    auto infixParselet = infixOps_.find(lex_.lookahead().first);
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

// Some simple ExpressionParselets
struct Literal : public ExpressionParselet {
  ExpressionNode* prefix(ExpressionParser* pParser) {
    auto token = pParser->getLexer().token();
    return new ExpressionNode(Token(token.first, token.second)); 
  }
};

struct Prefix : public ExpressionParselet {
  Prefix(int precedence) : ExpressionParselet(precedence) {} 
  ExpressionNode* prefix(ExpressionParser* pParser) {
    auto token = pParser->getLexer().token();
    return new ExpressionNode(Token(token.first, token.second), 0, pParser->parse());
  }
};

struct InfixLeft : public ExpressionParselet {
  InfixLeft(int precedence) : ExpressionParselet(precedence) {} 
  ExpressionNode* infix(ExpressionParser* pParser, ExpressionNode* pLeft) {
    auto token = pParser->getLexer().token();
    return new ExpressionNode(Token(token.first, token.second), pLeft, pParser->parse(precedence()));
  }
};

struct InfixRight : public ExpressionParselet {
  InfixRight(int precedence) : ExpressionParselet(precedence) {} 
  ExpressionNode* infix(ExpressionParser* pParser, ExpressionNode* pLeft) {
    auto token = pParser->getLexer().token();
    return new ExpressionNode(Token(token.first, token.second), pLeft, pParser->parse(precedence() - 1));
  }
};

struct GroupLeft : public ExpressionParselet {
  GroupLeft(int precedence = 0) : ExpressionParselet(precedence) {} 
  ExpressionNode* prefix(ExpressionParser* pParser) {
    auto token = pParser->getLexer().token();
    auto pGroup = new ExpressionNode(Token(token.first, token.second), 0, pParser->parse());
    pParser->getLexer().consume("CPAREN");
    return pGroup;
  }
};

struct ApplyLeft : public ExpressionParselet {
  ApplyLeft(int precedence) : ExpressionParselet(precedence) {} 
  ExpressionNode* infix(ExpressionParser* pParser, ExpressionNode* pLeft) {
    auto token = pParser->getLexer().token();
    auto pApply = new ExpressionNode(Token(token.first, token.second), pLeft, pParser->parse(0));
    pParser->getLexer().consume("CPAREN");
    return pApply;
  }
};

int main () {
  auto skip         = Regex::symbol(' ') | Regex::symbol('\n') | Regex::symbol('\t');
  auto digit        = Regex::range('0', '9');
  auto lower        = Regex::range('a', 'z');
  auto upper        = Regex::range('A', 'Z');
  auto alpha        = lower | upper;
  auto alphanum     = alpha | digit;
  auto number       = digit + *digit;
  auto identifier   = alpha + *(alphanum);
  auto eol          = Regex::symbol('\n');
  auto noteol       = Regex::exclude('\n');
  auto notpound     = Regex::exclude('#');
  auto linecomment  = Regex::symbol('#') + *noteol + eol;
  auto multicomment = Regex::match("##") + *notpound + Regex::match("##");
  auto comment      = linecomment; // | multicomment;
  auto string       = Regex::symbol('\"') + *Regex::exclude('\"') + Regex::symbol('\"');

  try {
    auto lexer = StringLexer::tokens({
      {"SKIP",        skip | comment},
      {"STRING",      string},
      {"SYM",         Regex::match(":") + identifier},
      {"NUM",         number},
      {"0IF",         Regex::match("if")},
      {"0WHILE",      Regex::match("while")},
      {"0UNTIL",      Regex::match("until")},
      {"0LOOP",       Regex::match("loop")},
      {"0WHEN",       Regex::match("when")},
      {"0DO",         Regex::match("do")},
      {"0VAL",        Regex::match("val")},
      {"0LET",        Regex::match("let")},
      {"0TEMPLATE",   Regex::match("template")},
      {"0TYPE",       Regex::match("type")},
      {"0TRUE",       Regex::match("true")},
      {"0FALSE",      Regex::match("false")},
      {"1ID",         identifier},
      {"OPAREN",      Regex::match('(')},
      {"CPAREN",      Regex::match(')')},
      {"OBRACK",      Regex::match('[')},
      {"CBRACK",      Regex::match(']')},
      {"OBRACE",      Regex::match('{')},
      {"CBRACE",      Regex::match('}')},
      {"ASSIGN",      Regex::match('=')},
      {"EQUALS",      Regex::match("==")},
      {"PLUS",        Regex::match('+')},
      {"MINUS",       Regex::match('-')},
      {"STAR",        Regex::match('*')},
      {"BSLASH",      Regex::match('\\')},
      {"SEMI",        Regex::match(';')},
      {"COLONS",      Regex::match("::")},
      {"COLON",       Regex::match(':')},
      {"DOT",         Regex::match('.')},
      {"COMMA",       Regex::match(',')},
      {"AT",          Regex::match('@')},
      {"ARROW",       Regex::match("->")},
      });

    std::cout << "realizing...";
    std::cout.flush();
    std::cout << "done." << std::endl;
    
    std::ifstream example("example");
    
    std::string test((std::istreambuf_iterator<char>(example)),
                      std::istreambuf_iterator<char>());

    // Define a simple expression parser
    ExpressionParser expr(lexer);
    expr.usePrefix(new Literal(),      "NUM");
    expr.usePrefix(new Literal(),      "1ID");
    expr.useInfix (new InfixLeft(10),  "PLUS");
    expr.useInfix (new InfixLeft(20),  "STAR");
    expr.usePrefix(new GroupLeft(0),   "OPAREN");
    expr.usePrefix(new Prefix(100),    "MINUS");
    expr.useInfix (new InfixLeft(200), "DOT");
    expr.useInfix (new ApplyLeft(500), "OPAREN");
    expr.useInfix (new InfixLeft(600), "COMMA");

    test = "foo.bar(12 + abc)";
    lexer.tokenize(test.begin(), test.end());

    auto pNode = expr.parse();
    pNode->print();
  } catch (std::exception& e) { 
    std::cout << "error: " << e.what() << std::endl;
  }
}
