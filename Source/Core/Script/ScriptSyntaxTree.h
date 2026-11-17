#pragma once

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "ScriptLexer.h"

/*
    The tree a script is parsed into: the structure the Parser builds and the Emitter reads. A
    script is one ClassDeclaration, holding its imports, the variables declared before or after the
    class (globals), the class's variables (members) and its functions. A function holds Statements
    (declare, assign, call, if, for, while, break, continue, return) and these hold Expressions
    (literal, name, node property, call, unary and binary operators). ValueType is a variable's
    type: int, float, double, bool or Node, void for a function with no result, and Inferred for a
    variable declared with 'let'. Each element keeps the line, column and length of its text, so
    the Emitter can mark errors the Parser could not see.

    Expressions and statements are not owned one by one. Every one the Parser makes is stored in
    the SyntaxArena of the ClassDeclaration, and the tree refers to them by pointer, so the whole
    tree is made as the Parser reads and freed in one step with the declaration.

    It is a header of its own so the Emitter does not need to include the Parser.
*/

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
using ExpressionPtr = Expression*;

struct Expression
{
    ExpressionKind kind = ExpressionKind::Literal;

    int    value        = 0;
    double decimalValue = 0.0;

    std::string name;
    std::string scope;

    TokenKind op = TokenKind::Null;

    ExpressionPtr left  = nullptr;
    ExpressionPtr right = nullptr;

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
using StatementPtr = Statement*;

struct Statement
{
    StatementKind kind = StatementKind::Return;

    ValueType   declaredType = ValueType::Inferred;
    std::string name;
    TokenKind   op = TokenKind::Assign;

    ExpressionPtr target = nullptr;
    ExpressionPtr value  = nullptr;

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

    ExpressionPtr value = nullptr;

    int line   = 1;
    int column = 1;
    int length = 1;
};

struct SyntaxArena
{
    std::deque<Expression> expressions;
    std::deque<Statement>  statements;
};

struct ClassDeclaration
{
    SyntaxArena arena;

    std::string name;

    std::vector<std::string> imports;

    std::vector<MemberDeclaration>   globals;
    std::vector<MemberDeclaration>   members;
    std::vector<FunctionDeclaration> functions;
};
