#include "ExpressionParser.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Support/raw_ostream.h"

struct SquidNode : public ExpressionNode {
  SquidNode(Token token, ExpressionNode* pLeft = 0, ExpressionNode* pRight = 0)
   : ExpressionNode(token, pLeft, pRight) {}
};

struct LiteralNode : public SquidNode {
  LiteralNode(Token token, ExpressionNode* pLeft = 0, ExpressionNode* pRight = 0)
   : SquidNode(token, pLeft, pRight) {}
};

struct OpNode : public SquidNode {
  OpNode(Token token, ExpressionNode* pLeft = 0, ExpressionNode* pRight = 0)
   : SquidNode(token, pLeft, pRight) {}
};

struct GroupNode : public SquidNode {
  GroupNode(Token token, ExpressionNode* pLeft = 0, ExpressionNode* pRight = 0)
   : SquidNode(token, pLeft, pRight) {}
};

// Some simple ExpressionParselets
struct Literal : public ExpressionParselet {
  ExpressionNode* prefix(ExpressionParser* pParser) {
    auto token = pParser->getLexer().token();
    return new LiteralNode(Token(token.type, token.lexeme)); 
  }
};

struct InfixLeft : public ExpressionParselet {
  InfixLeft(int precedence) : ExpressionParselet(precedence) {} 
  ExpressionNode* infix(ExpressionParser* pParser, ExpressionNode* pLeft) {
    auto token = pParser->getLexer().token();
    return new OpNode(Token(token.type, token.lexeme), pLeft, pParser->parse(precedence()));
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

using namespace llvm;

#include <cmath>
Value* eval(ExpressionNode* pNode, IRBuilder<> builder, Value* symbols) {
  if (!pNode) return 0;

  if (pNode->type() == "PLUS") {
    return builder.CreateBinOp(Instruction::Add, 
                               eval(pNode->left(), builder, symbols), 
                               eval(pNode->right(), builder, symbols));
  }

  if (pNode->type() == "STAR") {
    return builder.CreateBinOp(Instruction::Mul, 
                               eval(pNode->left(), builder, symbols), 
                               eval(pNode->right(), builder, symbols));
  }

  if (pNode->type() == "NUM") {
    return builder.getInt32(std::stoi(pNode->lexeme())); 
  }

  if (pNode->type() == "1ID") {
    std::cout << "loading " << pNode->lexeme() << std::endl;
    return builder.CreateLoad(symbols, pNode->lexeme()); 
  }

  if (pNode->type() == "OPAREN") {
    if (pNode->right() && !pNode->left()) {
      return eval(pNode->right(), builder, symbols); 
    } 
  }

  return 0;
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

  std::cout << "building lexer...";
  auto lexer = StringLexer::tokens({
    {"SKIP",        skip | comment},
    {"STRING",      string},
    {"SYM",         Regex::match(":") + identifier},
    {"NUM",         number},
    {"1ID",         identifier},
    {"OPAREN",      Regex::match('(')},
    {"CPAREN",      Regex::match(')')},
    {"PLUS",        Regex::match('+')},
    {"MINUS",       Regex::match('-')},
    {"STAR",        Regex::match('*')},
    {"FSLASH",      Regex::match('/')},
    });
  std::cout.flush();
  std::cout << "done." << std::endl;
  
  //std::ifstream example("example");
  //std::string test((std::istreambuf_iterator<char>(example)),
  //                  std::istreambuf_iterator<char>());

  // Define a simple expression parser
  std::cout << "building expression parser...";
  ExpressionParser expr(lexer);
  expr.usePrefix(new Literal(),      "NUM");
  expr.usePrefix(new Literal(),      "1ID");
  expr.usePrefix(new GroupLeft(0),   "OPAREN");
  expr.useInfix (new InfixLeft(10),  "PLUS");
  expr.useInfix (new InfixLeft(10),  "MINUS");
  expr.useInfix (new InfixLeft(20),  "STAR");
  expr.useInfix (new InfixLeft(20),  "FSLASH");
  std::cout.flush();
  std::cout << "done." << std::endl;

  auto test = std::string("3 * (4 + x)");
  lexer.tokenize(test.begin(), test.end());

  auto pNode = expr.parse();

  auto& context = llvm::getGlobalContext();
  // Module Construction
  Module* mod = new Module("code generation test", context);
  IRBuilder<> main_builder(context);

  // Declare i32 @main(i32, i8**)
  std::vector<Type*> mainArgs = {
    main_builder.getInt32Ty(),
    main_builder.getInt8PtrTy()->getPointerTo(),
  };

  FunctionType *funcType = llvm::FunctionType::get(main_builder.getInt32Ty(), mainArgs, false);
  Function *mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", mod);
  BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", mainFunc);
  main_builder.SetInsertPoint(entry);
  mainFunc->setCallingConv(CallingConv::C);
  mainFunc->addFnAttr(Attribute::StackProtect);


  Value* symbols = main_builder.CreateAlloca(main_builder.getInt32Ty(), 
                                             main_builder.getInt32(100), 
                                             "symbols");
  Value* retval = eval(pNode, main_builder, symbols);

  main_builder.CreateRet(retval);

  verifyModule(*mod, PrintMessageAction);

  PassManager PM;
  PM.add(createPrintModulePass(&outs()));
  PM.run(*mod);

  delete mod;
}
