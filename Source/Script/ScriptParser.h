#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "ScriptLexer.h"

enum class ValueType
{
    Int,
    Float,
    Double,
    Node,
    Void,
    Inferred
};

enum class ExpressionKind
{
    Literal,
    Decimal,
    None,
    Name,
    Field,
    Call,
    Unary,
    Binary
};

struct Expression;
using ExpressionPtr = std::unique_ptr<Expression>;

struct Expression
{
    ExpressionKind kind = ExpressionKind::Literal;

    int    value        = 0;
    double decimalValue = 0.0;

    std::string name;
    std::string scope;

    TokenKind op = TokenKind::Null;

    ExpressionPtr left;
    ExpressionPtr right;

    std::vector<ExpressionPtr> arguments;

    int line   = 1;
    int column = 1;
    int length = 1;
};

enum class StatementKind
{
    Declare,
    Assign,
    Call,
    If,
    For,
    While,
    Break,
    Continue,
    Return
};

struct Statement;
using StatementPtr = std::unique_ptr<Statement>;

struct Statement
{
    StatementKind kind = StatementKind::Return;

    ValueType   declaredType = ValueType::Inferred;
    std::string name;
    TokenKind   op = TokenKind::Assign;

    ExpressionPtr target;
    ExpressionPtr value;

    std::vector<StatementPtr> body;
    std::vector<StatementPtr> elseBody;

    bool hasElse = false;

    int line   = 1;
    int column = 1;
    int length = 1;
};

struct Parameter
{
    ValueType   type = ValueType::Int;
    std::string name;

    int line   = 1;
    int column = 1;
    int length = 1;
};

struct FunctionDeclaration
{
    ValueType   returnType = ValueType::Void;
    std::string name;

    std::vector<Parameter>    parameters;
    std::vector<StatementPtr> body;

    int line   = 1;
    int column = 1;
    int length = 1;
};

struct MemberDeclaration
{
    ValueType   type = ValueType::Int;
    std::string name;

    int line   = 1;
    int column = 1;
    int length = 1;
};

struct ClassDeclaration
{
    std::string name;

    std::vector<std::string>         imports;

    std::vector<MemberDeclaration>   members;
    std::vector<FunctionDeclaration> functions;
};

struct ParseFailure {};

class Parser
{
public:

    Parser(std::span<const Token> tokenList, std::vector<ScriptDiagnostic>& diagnosticList)
        : tokens(tokenList), diagnostics(diagnosticList) {}

    ClassDeclaration run();

private:

    void skipTerminators();
    const Token& current() const;
    void parseImports(ClassDeclaration& declaration);
    bool match(TokenKind kind);
    const Token& expect(TokenKind kind, const char* description);
    [[noreturn]] void fail(const std::string& message);
    void endStatement();
    void parseClassHeader(ClassDeclaration& declaration);
    void parseClassBody(ClassDeclaration& declaration);
    void parseClassMember(ClassDeclaration& declaration);
    ValueType parseType();
    void parseFunction(ClassDeclaration& declaration, ValueType returnType, const Token& nameToken);
    std::vector<StatementPtr> parseBlock();
    StatementPtr parseStatement();
    StatementPtr parseDeclaration();
    StatementPtr makeStatement(StatementKind kind, const Token& token);
    ExpressionPtr parseExpression();
    ExpressionPtr parseBinary(int minimumPrecedence);
    ExpressionPtr parseUnary();
    ExpressionPtr makeExpression(ExpressionKind kind, const Token& token);
    ExpressionPtr parsePostfix();
    ExpressionPtr parsePrimary();
    static int precedenceOf(TokenKind kind);
    StatementPtr parseIf();
    StatementPtr parseFor();
    StatementPtr parseWhile();
    StatementPtr parseSimple(StatementKind kind);
    StatementPtr parseReturn();
    StatementPtr parseAssignOrCall();
    void recover();

    std::span<const Token>         tokens;
    std::vector<ScriptDiagnostic>& diagnostics;

    std::size_t position = 0;
};
