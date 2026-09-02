#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "ScriptLexer.h"

namespace script {

enum class ExpressionKind
{
    Literal,
    Name,
    Member,
    Unary,
    Binary
};

struct Expression;
using ExpressionPtr = std::unique_ptr<Expression>;

struct Expression
{
    ExpressionKind kind = ExpressionKind::Literal;

    int value = 0;

    std::string name;
    std::string member;

    TokenKind op = TokenKind::End;

    ExpressionPtr left;
    ExpressionPtr right;

    int line   = 1;
    int column = 1;
    int length = 1;
};

enum class StatementKind
{
    Let,
    Assign,
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

    std::string name;
    TokenKind   op = TokenKind::Assign;

    ExpressionPtr value;

    std::vector<StatementPtr> body;
    std::vector<StatementPtr> elseBody;

    bool hasElse = false;

    int line   = 1;
    int column = 1;
    int length = 1;
};

class Parser
{
public:

    Parser(const std::vector<Token>& tokenList, std::vector<ScriptDiagnostic>& diagnosticList)
        : tokens(tokenList), diagnostics(diagnosticList) {}

    std::vector<StatementPtr> run();

private:

    const Token& current() const;

    bool match(TokenKind kind);

    const Token& expect(TokenKind kind, const char* description);

    void skipTerminators();
    void endStatement();
    void recover();

    [[noreturn]] void fail(const std::string& message);

    StatementPtr makeStatement(StatementKind kind, const Token& token);

    StatementPtr parseStatement();
    StatementPtr parseLet();
    StatementPtr parseAssign();
    StatementPtr parseIf();
    StatementPtr parseFor();
    StatementPtr parseWhile();
    StatementPtr parseSimple(StatementKind kind);
    StatementPtr parseReturn();

    std::vector<StatementPtr> parseBlock();

    ExpressionPtr makeExpression(ExpressionKind kind, const Token& token);

    static int precedenceOf(TokenKind kind);

    ExpressionPtr parseExpression();
    ExpressionPtr parseBinary(int minimumPrecedence);
    ExpressionPtr parseUnary();
    ExpressionPtr parsePrimary();

    const std::vector<Token>&      tokens;
    std::vector<ScriptDiagnostic>& diagnostics;

    std::size_t position = 0;
};

}
