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
  Token() {} 
  Token(std::string i, int l, TokenNodeGenerator f) 
    : value("")
    , id(i)
    , lbp(l)
    , gen(f) {
  }

  std::string        value;
  std::string        id;
  int                lbp; 
  TokenNodeGenerator gen;
};

// This really ought to be a ParseNode
struct ExpressionNode {
  ExpressionNode(const std::string& match, Token token, int lbp = 0) 
  : lbp_(lbp)
  , token_(token) 
  , match_ (match) {}

  virtual ExpressionNode* prefix(StringLexer& lex) {
    std::cout << ">> no prefix" << std::endl;
    //throw std::runtime_error("syntax error - no prefix");
    return 0;
  }

  virtual ExpressionNode* infix(ExpressionNode* pLeft, StringLexer& lex) {
    std::cout << ">> no infix" << std::endl;
    //throw std::runtime_error("syntax error - no infix");
    return 0;
  }

  virtual bool infix() { 
    return false; 
  }

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
  NilNode(const std::string& match = "wha?", Token token = Token("", 0, nullptr), int rbp = 0)
  : ExpressionNode(match, token, rbp) {}
};

struct Expression { // This should be a Parselet<>  
  Expression(StringLexer& lex, int rbp = 0) 
    : lex_                  (lex)
    , pCurrentExpressionNode_(0)
    , rbp_                  (rbp) { 
    pCurrentExpressionNode_     = nextPrefixExpressionNode();          
  }

  // Execute the parser and decorate the node with some context
  // TODO tokenizer state is inside...
  ExpressionNode* parse() {
    std::cout << "parsing" << std::endl;
    auto previousExpressionNode = pCurrentExpressionNode_;     
    pCurrentExpressionNode_     = nextPrefixExpressionNode();          
    auto pLeft                  = previousExpressionNode->prefix(lex_);     
    while (rbp_ < pCurrentExpressionNode_->lbp()) { 
      previousExpressionNode  = pCurrentExpressionNode_;    
      pCurrentExpressionNode_ = nextInfixExpressionNode();         
      pLeft                   = previousExpressionNode->infix(pLeft, lex_);
    }
  
    return pLeft;
  }

  ExpressionNode* nextPrefixExpressionNode() {
    auto tokenId = lex_.nextToken();
    auto token   = prefixOps_[tokenId.first];
    return token.gen(tokenId.first, token);
  }

  ExpressionNode* nextInfixExpressionNode() {
    auto tokenId = lex_.nextToken();
    auto token   = infixOps_[tokenId.first];
    return token.gen(tokenId.first, token);
  }

  template <typename NodeType>
  void usePrefix(std::string token, int rbp) {
    prefixOps_[token] = Token(
      token, 
      rbp,  
      [=](const std::string& match, Token token) -> ExpressionNode* {
       token.value = match;
       return new NodeType(match, token, rbp);
      }
    );
  }

  template <typename NodeType>
  void useInfix(std::string token, int rbp) {
    infixOps_[token] = Token(
      token, 
      rbp,  
      [=](const std::string& match, Token token) -> ExpressionNode* {
       token.value = match;
       return new NodeType(match, token, rbp);
      }
    );
  }

  StringLexer&                 lex_;
  ExpressionNode*              pCurrentExpressionNode_;
  int                          rbp_;
  std::map<std::string, Token> prefixOps_;
  std::map<std::string, Token> infixOps_;
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
  ExpressionNode* prefix(StringLexer& lex) {
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
  ExpressionNode* prefix(StringLexer& lex) {
    this->pRight.reset(Expression(lex, lbp()).parse());
    return this; 
  }
};

struct InfixLeft : public ExpressionNode {
  InfixLeft(const std::string& match, Token token, int rbp) 
  : ExpressionNode(match, token, rbp) {}
  ExpressionNode* infix(ExpressionNode* pLeft, StringLexer& lex) {
    this->pRight.reset(Expression(lex, lbp()).parse());
    this->pLeft.reset(pLeft);
    return this;
  }
  bool infix() { return true; }
};

struct InfixRight : public ExpressionNode {
  InfixRight(const std::string& match, Token token, int rbp) 
  : ExpressionNode(match, token, rbp) {}
  ExpressionNode* infix(ExpressionNode* pLeft, StringLexer& lex) {
    this->pRight.reset(Expression(lex, lbp()-1).parse());
    this->pLeft.reset(pLeft);
    return this;
  }
  bool infix() { return true; }
};

struct GroupLeft : public ExpressionNode {
  GroupLeft(const std::string& match, Token token, int rbp) 
  : ExpressionNode(match, token, rbp) {}
  ExpressionNode* prefix(StringLexer& lex) {
    this->pRight.reset(Expression(lex, 0).parse());
    //Tokenizer::consume(")");
    return this;
  }
};

struct ApplyLeft : public ExpressionNode {
  ApplyLeft(const std::string& match, Token token, int rbp) 
  : ExpressionNode(match, token, rbp) {}
  ExpressionNode* infix(ExpressionNode* pLeft, StringLexer& lex) {
    this->pRight.reset(Expression(lex, 0).parse());
    lex.consume(")");
    this->pLeft.reset(pLeft);
    return this;
  }
  bool infix() { return true; }
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
      {"SKIP",        skip},
      {"COMMENT",     comment},
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
      {"AT",          Regex::match('@')},
      {"ARROW",       Regex::match("->")},
      });

    std::cout << "realizing...";
    std::cout.flush();
    std::cout << "done." << std::endl;
    
    std::ifstream example("example");
    
    std::string test((std::istreambuf_iterator<char>(example)),
                      std::istreambuf_iterator<char>());

    Expression expr(lexer);
    expr.usePrefix<NilNode>    ("",               0);
    expr.usePrefix<LiteralNode>("DIGIT",          0);
    expr.useInfix<GroupLeft>   ("OPAREN(",        0);
    expr.useInfix<InfixLeft>   ("PLUS",           10);
    expr.useInfix<InfixLeft>   ("STAR",           20);
    expr.usePrefix<Prefix>     ("MINUS",          100);
    expr.useInfix<InfixLeft>   ("DOT",            200);
    expr.useInfix<ApplyLeft>   ("OPAREN",         500);

    test = "12+31";
    lexer.tokenize(test.begin(), test.end());

    auto pNode = expr.parse();

  } catch (std::exception& e) { 
    std::cout << "error: " << e.what() << std::endl;
  }
}
