template <typename S, typename T>
struct Lexer {
  typedef typename S::iterator              StreamIterator;
  typedef typename std::vector<T>::iterator TokenIterator;

  virtual TokenIterator currentToken() const = 0;
  virtual void          advance()            = 0;
};

struct Token {
  int         id;
  int         precedence;
  std::string value;
};

struct MyLexer : public Lexer<std::string, Token> {
  TokenIterator currentToken() const {
    return TokenIterator();
  }

  void          advance()  {
  } 
};
