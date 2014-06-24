#include "ExpressionParser.h"

// Some simple ExpressionParselets
struct Literal : public ExpressionParselet {
  ExpressionNode* prefix(ExpressionParser* pParser) {
    auto token = pParser->getLexer().token();
    return new ExpressionNode(Token(token.type, token.lexeme)); 
  }
};

struct Prefix : public ExpressionParselet {
  Prefix(int precedence) : ExpressionParselet(precedence) {} 
  ExpressionNode* prefix(ExpressionParser* pParser) {
    auto token = pParser->getLexer().token();
    return new ExpressionNode(Token(token.type, token.lexeme), 0, pParser->parse());
  }
};

struct InfixLeft : public ExpressionParselet {
  InfixLeft(int precedence) : ExpressionParselet(precedence) {} 
  ExpressionNode* infix(ExpressionParser* pParser, ExpressionNode* pLeft) {
    auto token = pParser->getLexer().token();
    return new ExpressionNode(Token(token.type, token.lexeme), pLeft, pParser->parse(precedence()));
  }
};

struct InfixRight : public ExpressionParselet {
  InfixRight(int precedence) : ExpressionParselet(precedence) {} 
  ExpressionNode* infix(ExpressionParser* pParser, ExpressionNode* pLeft) {
    auto token = pParser->getLexer().token();
    return new ExpressionNode(Token(token.type, token.lexeme), pLeft, pParser->parse(precedence() - 1));
  }
};

struct GroupLeft : public ExpressionParselet {
  GroupLeft(int precedence = 0) : ExpressionParselet(precedence) {} 
  ExpressionNode* prefix(ExpressionParser* pParser) {
    auto token = pParser->getLexer().token();
    auto pGroup = new ExpressionNode(Token(token.type, token.lexeme), 0, pParser->parse());
    pParser->getLexer().expect("CPAREN");
    return pGroup;
  }
};

struct ApplyLeft : public ExpressionParselet {
  ApplyLeft(int precedence) : ExpressionParselet(precedence) {} 
  ExpressionNode* infix(ExpressionParser* pParser, ExpressionNode* pLeft) {
    auto token = pParser->getLexer().token();
    auto pApply = new ExpressionNode(Token(token.type, token.lexeme), pLeft, pParser->parse(0));
    pParser->getLexer().expect("CPAREN");
    return pApply;
  }
};

#include <cmath>
double eval(ExpressionNode* pNode) {
  if (!pNode) return 0.0;

  if (pNode->type() == "PLUS") {
    return eval(pNode->left()) + eval(pNode->right()); 
  }

  if (pNode->type() == "NUM") {
    return std::stoi(pNode->lexeme()); 
  }

  if (pNode->type() == "OPAREN") {
    if (pNode->right() && !pNode->left()) {
      return eval(pNode->right()); 
    } else {
      if (pNode->left()->lexeme() == "sqrt") {
        return std::sqrt(eval(pNode->right())); 
      }
    }
  }
  
  return 0.0;
}

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
    
    //std::ifstream example("example");
    //std::string test((std::istreambuf_iterator<char>(example)),
    //                  std::istreambuf_iterator<char>());

    // Define a simple expression parser
    ExpressionParser expr(lexer);
    expr.usePrefix(new Literal(),      "NUM");
    expr.usePrefix(new Literal(),      "1ID");
    expr.usePrefix(new GroupLeft(0),   "OPAREN");
    expr.usePrefix(new Prefix   (100), "MINUS");
    expr.useInfix (new InfixLeft(1),   "COMMA");
    expr.useInfix (new InfixLeft(10),  "PLUS");
    expr.useInfix (new InfixLeft(20),  "STAR");
    expr.useInfix (new InfixLeft(200), "DOT");
    expr.useInfix (new ApplyLeft(500), "OPAREN");

    auto test = std::string("1+sqrt(4)");
    lexer.tokenize(test.begin(), test.end());

    auto pNode = expr.parse();

    std::cout << "eval(" << test << ") = " << eval(pNode) << std::endl;
  } catch (std::exception& e) { 
    std::cout << "error: " << e.what() << std::endl;
  }
}
