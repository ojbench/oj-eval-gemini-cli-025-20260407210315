#include <bits/stdc++.h>

#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// +------------------------------------+
// |              Values                |
// +------------------------------------+

class Value {
 public:
  virtual ~Value() {}
};
using ValuePtr = std::shared_ptr<Value>;

class IntValue : public Value {
 public:
  int value;
  IntValue(int value) : value(value) {}
};

class ArrayValue : public Value {
 public:
  int length;
  int *contents;
  ArrayValue(int length);
  ~ArrayValue() override;
};


// +------------------------------------+
// |        Program Structures          |
// +------------------------------------+


std::vector<uint64_t> global_trace;
class Program;
struct Context;

class BaseObject {
 public:
  virtual ~BaseObject() {}
  virtual std::string toString() const = 0;

  template <typename T>
  bool is() const {
    return dynamic_cast<const T *>(this) != nullptr;
  }
  template <typename T>
  T *as() {
    return dynamic_cast<T *>(this);
  }
};


// +------------------------------------+
// |           Expressions              |
// +------------------------------------+

class Expression : public BaseObject {
 public:
  virtual ValuePtr eval(Context &ctx) const = 0;
};

class IntegerLiteral : public Expression {
 public:
  int value;

  IntegerLiteral(int value) : value(value) {}
  std::string toString() const override;
  ValuePtr eval(Context &ctx) const override;
};

class Variable : public Expression {
 public:
  std::string name;

  Variable(std::string name) : name(std::move(name)) {}
  std::string toString() const override;
  ValuePtr eval(Context &ctx) const override;
};

class CallExpression : public Expression {
 public:
  std::string func;
  std::vector<Expression *> args;

  CallExpression(std::string func, std::vector<Expression *> args)
      : func(std::move(func)), args(std::move(args)) {}
  std::string toString() const override;
  ValuePtr eval(Context &ctx) const override;
};


// +------------------------------------+
// |           Statements               |
// +------------------------------------+

class Statement : public BaseObject {
 public:
  virtual void eval(Context &ctx) const = 0;
};

class ExpressionStatement : public Statement {
 public:
  Expression *expr;

  ExpressionStatement(Expression *expr) : expr(expr) {}
  std::string toString() const override;
  void eval(Context &ctx) const override;
};

class SetStatement : public Statement {
 public:
  Variable *name;
  Expression *value;

  SetStatement(Variable *name, Expression *value) : name(name), value(value) {}
  std::string toString() const override;
  void eval(Context &ctx) const override;
};

class IfStatement : public Statement {
 public:
  Expression *condition;
  Statement *body;

  IfStatement(Expression *condition, Statement *body)
      : condition(condition), body(body) {}
  std::string toString() const override;
  void eval(Context &ctx) const override;
};

class ForStatement : public Statement {
 public:
  Statement *init;
  Expression *test;
  Statement *update;
  Statement *body;

  ForStatement(Statement *init, Expression *test, Statement *update,
               Statement *body)
      : init(init), test(test), update(update), body(body) {}
  std::string toString() const override;
  void eval(Context &ctx) const override;
};

class BlockStatement : public Statement {
 public:
  std::vector<Statement *> body;

  BlockStatement(std::vector<Statement *> body) : body(std::move(body)) {}
  std::string toString() const override;
  void eval(Context &ctx) const override;
};

class ReturnStatement : public Statement {
 public:
  Expression *value;

  ReturnStatement(Expression *value) : value(value) {}
  std::string toString() const override;
  void eval(Context &ctx) const override;
};


// +------------------------------------+
// |        Global Constructs           |
// +------------------------------------+

class FunctionDeclaration : public BaseObject {
 public:
  std::string name;
  std::vector<Variable *> params;
  Statement *body;

  FunctionDeclaration(std::string name, std::vector<Variable *> params,
                      Statement *body)
      : name(std::move(name)), params(std::move(params)), body(body) {}
  std::string toString() const override;
};

class Program : public BaseObject {
 public:
  std::vector<FunctionDeclaration *> body;
  std::unordered_map<std::string, FunctionDeclaration *> index;

  Program(std::vector<FunctionDeclaration *> body);
  std::string toString() const override;
  int eval(int timeLimit, std::istream &is = std::cin, std::ostream &os = std::cout);
};


// +------------------------------------+
// |            Exceptions              |
// +------------------------------------+

class EvalError : public std::exception {
 public:
  const BaseObject *location;
  std::string reason;

  EvalError(const BaseObject *location, const std::string &reason_)
      : location(location) {
    if (location == nullptr) {
      reason = reason_;
      return;
    }
    reason = "At " + location->toString() + ":\n" + reason_;
  }
  const char *what() const noexcept override { return reason.c_str(); }
};
class SyntaxError : public EvalError {
 public:
  SyntaxError(const BaseObject *location, const std::string &reason)
      : EvalError(location, "Syntax error: " + reason) {}
};
class RuntimeError : public EvalError {
 public:
  RuntimeError(const BaseObject *location, const std::string &reason)
      : EvalError(location, "Runtime error: " + reason) {}
};


// +------------------------------------+
// |               Parser               |
// +------------------------------------+

BaseObject *scan(std::istream &is);
Program *scanProgram(std::istream &is);

// +------------------------------------+
// |   Variables and Helper Functions   |
// +------------------------------------+

extern const int kIdMaxLength;
extern const std::unordered_set<std::string> keywords;
extern const std::unordered_set<std::string> builtinFunctions;

bool isTruthy(const BaseObject *ctx, ValuePtr value);
std::string indent(const std::string &s);
bool isValidIdentifier(const std::string &name);
void removeWhitespaces(std::istream &is);
void expectClosingParens(std::istream &is);
std::string scanToken(std::istream &is);
std::string scanIdentifier(std::istream &is);


template <typename T>
class Visitor {
 public:
  virtual T visitProgram(Program *node) { return {}; }
  virtual T visitFunctionDeclaration(FunctionDeclaration *node) { return {}; }

  virtual T visitStatement(Statement *node) {
    if (node->is<ExpressionStatement>()) {
      return visitExpressionStatement(node->as<ExpressionStatement>());
    } else if (node->is<SetStatement>()) {
      return visitSetStatement(node->as<SetStatement>());
    } else if (node->is<IfStatement>()) {
      return visitIfStatement(node->as<IfStatement>());
    } else if (node->is<ForStatement>()) {
      return visitForStatement(node->as<ForStatement>());
    } else if (node->is<BlockStatement>()) {
      return visitBlockStatement(node->as<BlockStatement>());
    } else if (node->is<ReturnStatement>()) {
      return visitReturnStatement(node->as<ReturnStatement>());
    }
    throw SyntaxError(node, "Unknown type");
  }
  virtual T visitExpressionStatement(ExpressionStatement *node) {
    return visitExpression(node->expr);
  }
  virtual T visitSetStatement(SetStatement *node) { return {}; }
  virtual T visitIfStatement(IfStatement *node) { return {}; }
  virtual T visitForStatement(ForStatement *node) { return {}; }
  virtual T visitBlockStatement(BlockStatement *node) { return {}; }
  virtual T visitReturnStatement(ReturnStatement *node) { return {}; }

  virtual T visitExpression(Expression *node) {
    if (node->is<IntegerLiteral>()) {
      return visitIntegerLiteral(node->as<IntegerLiteral>());
    } else if (node->is<Variable>()) {
      return visitVariable(node->as<Variable>());
    } else if (node->is<CallExpression>()) {
      return visitCallExpression(node->as<CallExpression>());
    }
    throw SyntaxError(node, "Unknown type");
  }
  virtual T visitIntegerLiteral(IntegerLiteral *node) { return {}; }
  virtual T visitVariable(Variable *node) { return {}; }
  virtual T visitCallExpression(CallExpression *node) { return {}; }
};


class Transform {
 public:
  virtual Program *transformProgram(Program *node) {
    std::vector<FunctionDeclaration *> body;
    for (auto decl : node->body) {
      body.push_back(transformFunctionDeclaration(decl));
    }
    return new Program(body);
  }

  virtual FunctionDeclaration *transformFunctionDeclaration(FunctionDeclaration *node) {
    std::vector<Variable *> params;
    for (auto param : node->params) {
      params.push_back(transformVariable(param));
    }
    return new FunctionDeclaration(node->name, params, transformStatement(node->body));
  }

  virtual Statement *transformStatement(Statement *node) {
    if (node->is<ExpressionStatement>()) {
      return transformExpressionStatement(node->as<ExpressionStatement>());
    } else if (node->is<SetStatement>()) {
      return transformSetStatement(node->as<SetStatement>());
    } else if (node->is<IfStatement>()) {
      return transformIfStatement(node->as<IfStatement>());
    } else if (node->is<ForStatement>()) {
      return transformForStatement(node->as<ForStatement>());
    } else if (node->is<BlockStatement>()) {
      return transformBlockStatement(node->as<BlockStatement>());
    } else if (node->is<ReturnStatement>()) {
      return transformReturnStatement(node->as<ReturnStatement>());
    }
    throw SyntaxError(node, "Unknown type");
  }

  virtual Statement *transformExpressionStatement(ExpressionStatement *node) {
    return new ExpressionStatement(transformExpression(node->expr));
  }

  virtual Statement *transformSetStatement(SetStatement *node) {
    return new SetStatement(transformVariable(node->name), transformExpression(node->value));
  }

  virtual Statement *transformIfStatement(IfStatement *node) {
    return new IfStatement(transformExpression(node->condition), transformStatement(node->body));
  }

  virtual Statement *transformForStatement(ForStatement *node) {
    return new ForStatement(transformStatement(node->init), transformExpression(node->test),
                            transformStatement(node->update), transformStatement(node->body));
  }

  virtual Statement *transformBlockStatement(BlockStatement *node) {
    std::vector<Statement *> body;
    for (auto stmt : node->body) {
      body.push_back(transformStatement(stmt));
    }
    return new BlockStatement(body);
  }

  virtual Statement *transformReturnStatement(ReturnStatement *node) {
    return new ReturnStatement(transformExpression(node->value));
  }

  virtual Expression *transformExpression(Expression *node) {
    if (node->is<IntegerLiteral>()) {
      return transformIntegerLiteral(node->as<IntegerLiteral>());
    } else if (node->is<Variable>()) {
      return transformVariable(node->as<Variable>());
    } else if (node->is<CallExpression>()) {
      return transformCallExpression(node->as<CallExpression>());
    }
    throw SyntaxError(node, "Unknown type");
  }

  virtual Expression *transformIntegerLiteral(IntegerLiteral *node) {
    return new IntegerLiteral(node->value);
  }

  virtual Variable *transformVariable(Variable *node) {
    return new Variable(node->name);
  }

  virtual Expression *transformCallExpression(CallExpression *node) {
    std::vector<Expression *> args;
    for (auto arg : node->args) {
      args.push_back(transformExpression(arg));
    }
    return new CallExpression(node->func, args);
  }
};

#include <cctype>
#include <iostream>
#include <stack>
#include <type_traits>
#include <unordered_set>

const int kIdMaxLength = 255;
const std::unordered_set<std::string> keywords = {
    "set", "if", "for", "block", "return", "function",
};
const std::unordered_set<std::string> builtinFunctions = {
    "+", "-", "*", "/", "%", "<", ">", "<=", ">=", "==", "!=", "||", "&&", "!",
    "scan", "print", "array.create", "array.get", "array.set", "array.scan", "array.print",
};

bool isTruthy(const BaseObject *ctx, ValuePtr value) {
  auto iv = std::dynamic_pointer_cast<IntValue>(value);
  if (iv == nullptr) {
    throw RuntimeError(ctx, "Type error: if condition should be an int");
  }
  return iv->value != 0;
}

ArrayValue::ArrayValue(int length) : length(length) {
  if (length > 1000000) throw RuntimeError(nullptr, "Out of memory");
  contents = new int[length];
  for (int i = 0; i < length; ++i) contents[i] = 0;
}
ArrayValue::~ArrayValue() { delete[] contents; }

struct VariableSet {
  std::unordered_map<std::string, ValuePtr> values;
  ValuePtr getOrThrow(const BaseObject *ctx, const std::string &name) {
    if (values.count(name) == 0) {
      throw RuntimeError(ctx, "Use of undefined variable: " + name);
    }
    if (values[name] == nullptr) {
      throw RuntimeError(ctx, "Use of uninitialized variable: " + name);
    }
    return values[name];
  }
};

struct Context {
  std::istream &is;
  std::ostream &os;
  std::stack<VariableSet> callStack;
  Program *program;
  int timeLeft;

  VariableSet &currentFrame() { return callStack.top(); }
  ValuePtr getOrThrow(const BaseObject *ctx, const std::string &name) {
    return currentFrame().getOrThrow(ctx, name);
  }
  void set(const BaseObject *ctx, const std::string &name, ValuePtr value) {
    if (program->index.count(name) > 0 || builtinFunctions.count(name) > 0) {
      throw RuntimeError(ctx, "Assigning to function " + name);
    }
    currentFrame().values[name] = value;
  }
  void tick() {
    --timeLeft;
    if (timeLeft < 0) {
      throw RuntimeError(nullptr, "Time limit exceeded");
    }
  }
};

std::string IntegerLiteral::toString() const { return std::to_string(value); }
ValuePtr IntegerLiteral::eval(Context &ctx) const {
  ctx.tick();
  return std::make_shared<IntValue>(value);
}

std::string Variable::toString() const { return name; }
ValuePtr Variable::eval(Context &ctx) const {
  ctx.tick();
  return ctx.getOrThrow(this, name);
}

std::string ExpressionStatement::toString() const { return expr->toString(); }
void ExpressionStatement::eval(Context &ctx) const { expr->eval(ctx); }

struct ReturnFromCall {
  ValuePtr value;
};
std::string CallExpression::toString() const {
  std::string str = std::string("(") + func;
  for (const auto &arg : args) {
    str += " ";
    str += arg->toString();
  }
  str += ")";
  return str;
}
ValuePtr CallExpression::eval(Context &ctx) const {
  ctx.tick();
  global_trace.push_back(std::hash<std::string>()(func));
  std::vector<ValuePtr> argValues;
  for (const auto &arg : args) {
    argValues.push_back(arg->eval(ctx));
  }

  auto requireArity = [&](int arity) {
    if (args.size() != arity) {
      throw RuntimeError(this, "Function arity mismatch at " + func);
    }
  };
  auto readInt = [&](int ix) {
    auto iv = std::dynamic_pointer_cast<IntValue>(argValues[ix]);
    if (!iv) throw RuntimeError(this, "Type error: int expected");
    return iv->value;
  };

  if (func == "+") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x + y);
  } else if (func == "-") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x - y);
  } else if (func == "*") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x * y);
  } else if (func == "/") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    if (y == 0) {
      throw RuntimeError(this, "Divide by zero");
    }
    return std::make_shared<IntValue>(x / y);
  } else if (func == "%") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    if (y == 0) {
      throw RuntimeError(this, "Mod by zero");
    }
    return std::make_shared<IntValue>(x % y);
  } else if (func == "<") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x < y);
  } else if (func == ">") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x > y);
  } else if (func == "<=") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x <= y);
  } else if (func == ">=") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x >= y);
  } else if (func == "==") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x == y);
  } else if (func == "!=") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x != y);
  } else if (func == "||") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x || y);
  } else if (func == "&&") {
    requireArity(2);
    int x = readInt(0), y = readInt(1);
    return std::make_shared<IntValue>(x && y);
  } else if (func == "!") {
    requireArity(1);
    int x = readInt(0);
    return std::make_shared<IntValue>(!x);
  } else if (func == "scan") {
    requireArity(0);
    int x;
    ctx.is >> x;
    return std::make_shared<IntValue>(x);
  } else if (func == "print") {
    requireArity(1);
    ctx.os << readInt(0) << '\n';
    return std::make_shared<IntValue>(0);
  } else if (func == "array.create") {
    requireArity(1);
    return std::make_shared<ArrayValue>(readInt(0));
  } else if (func == "array.scan") {
    requireArity(1);
    int length = readInt(0);
    auto array = std::make_shared<ArrayValue>(length);
    for (int i = 0; i < length; ++i) {
      ctx.is >> array->contents[i];
    }
    return array;
  } else if (func == "array.print") {
    requireArity(1);
    auto array = std::dynamic_pointer_cast<ArrayValue>(argValues[0]);
    if (!array)
      throw RuntimeError(this, "Type error at array.get: array expected");
    for (int i = 0; i < array->length; ++i) {
      ctx.os << array->contents[i] << std::endl;
    }
    return std::make_shared<IntValue>(0);
  } else if (func == "array.get") {
    requireArity(2);
    auto array = std::dynamic_pointer_cast<ArrayValue>(argValues[0]);
    int index = readInt(1);
    if (!array)
      throw RuntimeError(this, "Type error at array.get: array expected");
    if (index >= array->length || index < 0) {
      throw RuntimeError(this, "Index out of bounds at array.get");
    }
    global_trace.push_back(1ULL ^ std::hash<int>()(index) ^ std::hash<int>()(array->contents[index]));
    return std::make_shared<IntValue>(array->contents[index]);
  } else if (func == "array.set") {
    requireArity(3);
    auto array = std::dynamic_pointer_cast<ArrayValue>(argValues[0]);
    int index = readInt(1);
    int value = readInt(2);
    if (!array)
      throw RuntimeError(this, "Type error at array.set: array expected");
    if (index >= array->length || index < 0) {
      throw RuntimeError(this, "Index out of bounds at array.set");
    }
    global_trace.push_back(2ULL ^ std::hash<int>()(index) ^ std::hash<int>()(value));
    array->contents[index] = value;
    return std::make_shared<IntValue>(0);
  }

  auto *funcObject = ctx.program->index[func];
  if (!funcObject) throw RuntimeError(this, "No such function: " + func);
  requireArity(funcObject->params.size());

  ctx.callStack.push({});
  for (int i = 0; i < args.size(); ++i) {
    const auto &name = funcObject->params[i];
    if (ctx.program->index.count(name->name) > 0) {
      throw RuntimeError(
          this, "Function parameter name is global identifier: " + name->name);
    }
    ctx.set(this, name->name, argValues[i]);
  }

  try {
    funcObject->body->eval(ctx);
  } catch (ReturnFromCall r) {
    ctx.callStack.pop();
    return r.value;
  }

  ctx.callStack.pop();
  return std::make_shared<IntValue>(0);
}

std::string indent(const std::string &s) {
  std::string res = "  ";
  for (char ch : s) {
    res += ch;
    if (ch == '\n') {
      res += "  ";
    }
  }
  return res;
}

std::string SetStatement::toString() const {
  return std::string("(set ") + name->toString() + " " + value->toString() + ")";
}
void SetStatement::eval(Context &ctx) const {
  ctx.tick();
  ctx.set(this, name->name, value->eval(ctx));
}

std::string IfStatement::toString() const {
  return std::string("(if ") + condition->toString() + "\n" +
         indent(body->toString()) + ")";
}
void IfStatement::eval(Context &ctx) const {
  ctx.tick();
  bool ok = isTruthy(this, condition->eval(ctx));
  if (ok) {
    body->eval(ctx);
  }
}

std::string ForStatement::toString() const {
  return std::string("(for\n") + indent(init->toString()) + "\n" +
         indent(test->toString()) + "\n" + indent(update->toString()) + "\n" +
         indent(body->toString()) + ")";
}
void ForStatement::eval(Context &ctx) const {
  ctx.tick();
  for (init->eval(ctx); isTruthy(this, test->eval(ctx)); update->eval(ctx)) {
    body->eval(ctx);
  }
}

std::string BlockStatement::toString() const {
  std::string str = "(block";
  for (const auto &stmt : body) {
    str += "\n";
    str += indent(stmt->toString());
  }
  str += ")";
  return str;
}
void BlockStatement::eval(Context &ctx) const {
  for (auto stmt : body) {
    stmt->eval(ctx);
  }
}

std::string ReturnStatement::toString() const {
  return std::string("(return ") + value->toString() + ")";
}
void ReturnStatement::eval(Context &ctx) const {
  throw ReturnFromCall{value->eval(ctx)};
}

std::string FunctionDeclaration::toString() const {
  std::string str = "(function (";
  str += name;
  for (const auto &param : params) {
    str += " ";
    str += param->toString();
  }
  str += ")\n";
  str += indent(body->toString());
  str += ")";
  return str;
}

Program::Program(std::vector<FunctionDeclaration *> body)
    : body(std::move(body)) {
  for (auto el : this->body) {
    const auto &name = el->name;
    if (builtinFunctions.count(name) > 0) {
      throw SyntaxError(nullptr, "Redefining built-in function: " + name);
    }
    if (index.count(name) > 0) {
      throw SyntaxError(nullptr, "Duplicate function declaration: " + name);
    }
    index[name] = el;
  }
}
std::string Program::toString() const {
  std::string str;
  for (auto el : body) {
    str += el->toString();
    str += "\n\n";
  }
  return str;
}
int Program::eval(int timeLimit, std::istream &is, std::ostream &os) {
  Context ctx{
      .is = is,
      .os = os,
      .callStack = {},
      .program = this,
      .timeLeft = timeLimit,
  };
  CallExpression("main", {}).eval(ctx);
  return timeLimit - ctx.timeLeft;
}

bool isValidIdentifier(const std::string &name) {
  if (name.length() > kIdMaxLength) return false;
  if (name.empty()) return false;
  if (isdigit(name[0])) return false;
  if (name[0] == '-') {
    if (name.length() == 1) return true;
    bool isNumber = true;
    for (int i = 1; i < name.length(); ++i) {
      if (!isdigit(name[i])) {
        isNumber = false;
        break;
      }
    }
    if (isNumber) return false;
  }
  for (char ch : name) {
    if (ch == ')' || ch == '(' || ch == ';') return false;
    if (!isgraph(ch)) return false;
  }
  if (keywords.count(name) > 0) return false;
  return true;
}

void removeWhitespaces(std::istream &is) {
  while (is && isspace(is.peek())) is.get();
  if (is.peek() == ';') {
    int ch;
    do {
      ch = is.get();
    } while (ch != EOF && ch != '\n');
    removeWhitespaces(is);
  }
}

void expectClosingParens(std::istream &is) {
  removeWhitespaces(is);
  int ch = is.get();
  if (ch != ')') {
    throw SyntaxError(
        nullptr, std::string("Closing parenthesis expected, got ") + char(ch));
  }
}

std::string scanToken(std::istream &is) {
  removeWhitespaces(is);
  std::string token;
  for (int next = is.peek(); !isspace(next) && next != ')' && next != ';';
       next = is.peek()) {
    token += is.get();
  }
  return token;
}

std::string scanIdentifier(std::istream &is) {
  auto name = scanToken(is);
  if (!isValidIdentifier(name))
    throw SyntaxError(nullptr, "Invalid identifier: " + name);
  return name;
}

template <typename T>
static T *scanT(std::istream &is) {
  auto *construct = scan(is);
  if (construct == nullptr) throw SyntaxError(nullptr, "Unexpected EOF");
  if (!construct->is<T>()) {
    if constexpr (std::is_same_v<T, Statement>) {
      if (construct->is<Expression>()) {
        return new ExpressionStatement(construct->as<Expression>());
      }
    }
    throw SyntaxError(nullptr, std::string("Wrong construct type; ") +
                                   typeid(*construct).name() + " found, " +
                                   typeid(T).name() + " expected");
  }
  return construct->as<T>();
}

BaseObject *scan(std::istream &is) {
  // ignore whitespaces
  removeWhitespaces(is);
  if (!is || is.peek() == EOF) return nullptr;
  if (is.peek() != '(') {
    // variable or literal
    auto name = scanToken(is);
    if (name.empty()) return nullptr;
    if (name[0] == '-') {
      bool isLiteral = true;
      if (name.length() == 1) {
        isLiteral = false;
      } else {
        for (char ch : name.substr(1)) {
          if (!isdigit(ch)) {
            isLiteral = false;
            break;
          }
        }
      }
      if (isLiteral) {
        int value = std::stoi(name);
        return new IntegerLiteral(value);
      }
    }
    if (isdigit(name[0])) {
      for (char ch : name) {
        if (!isdigit(ch)) {
          throw SyntaxError(nullptr, "Invalid literal: " + name);
        }
      }
      int value = std::stoi(name);
      return new IntegerLiteral(value);
    }
    if (isValidIdentifier(name)) {
      return new Variable(name);
    }
    throw SyntaxError(nullptr, "Invalid identifier: " + name);
  }
  is.get();

  auto type = scanToken(is);
  if (type == "set") {
    auto name = scanIdentifier(is);
    auto *value = scanT<Expression>(is);
    expectClosingParens(is);
    return new SetStatement(new Variable(name), value->as<Expression>());
  } else if (type == "if") {
    auto *cond = scanT<Expression>(is);
    auto *body = scanT<Statement>(is);
    expectClosingParens(is);
    return new IfStatement(cond, body);
  } else if (type == "for") {
    auto *init = scanT<Statement>(is);
    auto *test = scanT<Expression>(is);
    auto *update = scanT<Statement>(is);
    auto *body = scanT<Statement>(is);
    expectClosingParens(is);
    return new ForStatement(init, test, update, body);
  } else if (type == "block") {
    std::vector<Statement *> body;
    removeWhitespaces(is);
    while (is.peek() != ')') {
      body.push_back(scanT<Statement>(is));
      removeWhitespaces(is);
    }
    expectClosingParens(is);
    return new BlockStatement(body);
  } else if (type == "return") {
    auto *value = scanT<Expression>(is);
    expectClosingParens(is);
    return new ReturnStatement(value);
  } else if (type == "function") {
    removeWhitespaces(is);
    if (is.get() != '(') {
      throw SyntaxError(nullptr, "Opening parenthesis expected");
    }
    auto name = scanIdentifier(is);
    std::vector<Variable *> params;
    removeWhitespaces(is);
    while (is.peek() != ')') {
      params.push_back(new Variable(scanIdentifier(is)));
      removeWhitespaces(is);
    }
    expectClosingParens(is);
    auto *body = scanT<Statement>(is);
    expectClosingParens(is);
    return new FunctionDeclaration(name, params, body);
  } else {
    // call expression
    auto &name = type;
    if (!isValidIdentifier(name))
      throw SyntaxError(nullptr, "Invalid identifier: " + name);
    std::vector<Expression *> args;
    removeWhitespaces(is);
    while (is.peek() != ')') {
      args.push_back(scanT<Expression>(is));
      removeWhitespaces(is);
    }
    expectClosingParens(is);
    return new CallExpression(name, args);
  }
}

Program *scanProgram(std::istream &is) {
  std::vector<FunctionDeclaration *> body;
  while (true) {
    auto *el = scan(is);
    if (el == nullptr) break;
    if (!el->is<FunctionDeclaration>()) {
      if (el->is<Variable>() && el->as<Variable>()->name == "endprogram") {
        break;
      }
      throw SyntaxError(nullptr, "Invalid program element");
    }
    body.push_back(el->as<FunctionDeclaration>());
  }
  return new Program(body);
}

class MyCheat : public Transform {
 public:
  std::unordered_map<std::string, std::string> rename_map;
  std::string random_name(const std::string& orig) {
    if (orig == "main") return "main";
    if (rename_map.count(orig)) return rename_map[orig];
    std::string res = "O";
    for (int i = 0; i < 10; ++i) res += (rand() % 2 ? 'O' : '0');
    return rename_map[orig] = res;
  }
  
  bool is_builtin(const std::string& name) {
    static std::unordered_set<std::string> builtins = {
      "+", "-", "*", "/", "%", "<", ">", "<=", ">=", "==", "!=", "||", "&&", "!",
      "scan", "print", "array.create", "array.scan", "array.print", "array.get", "array.set"
    };
    return builtins.count(name);
  }

  FunctionDeclaration *transformFunctionDeclaration(FunctionDeclaration *node) override {
    std::vector<Variable *> params;
    for (auto param : node->params) {
      params.push_back(transformVariable(param));
    }
    return new FunctionDeclaration(random_name(node->name), params, transformStatement(node->body));
  }

  Variable *transformVariable(Variable *node) override {
    return new Variable(random_name(node->name));
  }

  Expression *transformCallExpression(CallExpression *node) override {
    std::vector<Expression *> args;
    for (auto arg : node->args) {
      args.push_back(transformExpression(arg));
    }
    std::string func_name = node->func;
    if (!is_builtin(func_name)) {
      func_name = random_name(func_name);
    }
    return new CallExpression(func_name, args);
  }

  Statement *transformBlockStatement(BlockStatement *node) override {
    std::vector<Statement *> body;
    for (auto stmt : node->body) {
      body.push_back(new SetStatement(new Variable(random_name("dummy")), new IntegerLiteral(rand() % 100)));
      body.push_back(transformStatement(stmt));
    }
    return new BlockStatement(body);
  }
  
  Expression *transformIntegerLiteral(IntegerLiteral *node) override {
    if (rand() % 2 == 0) {
      return new CallExpression("+", {new IntegerLiteral(node->value), new IntegerLiteral(0)});
    }
    return new IntegerLiteral(node->value);
  }

  Program *transformProgram(Program *node) override {
    std::vector<FunctionDeclaration *> body;
    for (auto decl : node->body) {
      body.push_back(transformFunctionDeclaration(decl));
    }
    std::shuffle(body.begin(), body.end(), std::mt19937(std::random_device()()));
    return new Program(body);
  }
};

double jaccard(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b) {
    if (a.empty() && b.empty()) return 1.0;
    if (a.empty() || b.empty()) return 0.0;
    std::unordered_map<uint64_t, int> count_a, count_b;
    for (int i = 0; i + 2 < a.size(); ++i) {
        uint64_t h = a[i] ^ (a[i+1] << 13) ^ (a[i+2] << 27);
        count_a[h]++;
    }
    for (int i = 0; i + 2 < b.size(); ++i) {
        uint64_t h = b[i] ^ (b[i+1] << 13) ^ (b[i+2] << 27);
        count_b[h]++;
    }
    int intersection = 0;
    int union_size = 0;
    for (auto& kv : count_a) {
        int cb = count_b[kv.first];
        intersection += std::min(kv.second, cb);
        union_size += std::max(kv.second, cb);
    }
    for (auto& kv : count_b) {
        if (count_a.count(kv.first) == 0) {
            union_size += kv.second;
        }
    }
    if (union_size == 0) return 1.0;
    return (double)intersection / union_size;
}

int main() {
  srand(time(NULL));
  Program *prog1 = scanProgram(std::cin);
  std::cin >> std::ws;
  if (std::cin.eof()) {
    // cheat
    auto cheat = MyCheat().transformProgram(prog1);
    std::cout << cheat->toString();
  } else {
    // anticheat
    Program *prog2 = scanProgram(std::cin);
    std::string input;
    int c;
    while ((c = std::cin.get()) != EOF) {
      input += c;
    }

    std::istringstream iss1(input);
    std::ostringstream oss1;
    global_trace.clear();
    try {
        prog1->eval(1000000, iss1, oss1);
    } catch (...) {}
    std::vector<uint64_t> trace1 = global_trace;

    std::istringstream iss2(input);
    std::ostringstream oss2;
    global_trace.clear();
    try {
        prog2->eval(1000000, iss2, oss2);
    } catch (...) {}
    std::vector<uint64_t> trace2 = global_trace;

    double score = jaccard(trace1, trace2);
    if (score > 0.5) {
        std::cout << 0.5 + score / 2.0 << std::endl;
    } else if (score < 0.1) {
        std::cout << 0.0 << std::endl;
    } else {
        std::cout << score << std::endl;
    }
  }
  return 0;
}
