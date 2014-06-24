#include <Lexer.h>

//------------------------------------------------------------------------------

int main() {
  std::cout << "Lexer Test" << std::endl;

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

    lexer.tokenize(test.begin(), test.end());
    while(!lexer.eof()) {
      auto token = lexer.nextToken();
      if (token.type != "SKIP" && token.type != "") {
        auto lexeme = token.lexeme;
        lexeme.erase(std::remove(lexeme.begin(), lexeme.end(), '\n'), lexeme.end());
        std::cout << "read: " << std::setw(10) << token.type << " = \'" << lexeme << "\'" << std::endl;
      }
    }
  } catch (std::exception& e) { 
    std::cout << "error: " << e.what() << std::endl;
  }
}
