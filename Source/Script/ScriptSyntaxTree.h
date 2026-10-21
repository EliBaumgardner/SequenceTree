#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ScriptLexer.h"

enum class ValueType
{
    Int,
    Float,
    Double,
    Bool,
    Node,
    Void,
    Inferred
};

enum class ExpressionKind
{
    Literal,
    Decimal,
    Boolean,
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

    ExpressionPtr value;

    int line   = 1;
    int column = 1;
    int length = 1;
};

struct ClassDeclaration
{
    std::string name;

    std::vector<std::string> imports;

    std::vector<MemberDeclaration>   globals;
    std::vector<MemberDeclaration>   members;
    std::vector<FunctionDeclaration> functions;
};
