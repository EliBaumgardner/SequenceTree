#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include "ScriptLexer.h"
#include "ScriptSyntaxTree.h"

struct ParseFailure {};

class Parser
{
public:

    Parser(std::span<const Token> tokenList, std::vector<ScriptDiagnostic>& diagnosticList)
        : tokens(tokenList), diagnostics(diagnosticList) {}

    ClassDeclaration run();

private:

    void parseImports    (ClassDeclaration& declaration);
    void parseGlobals    (ClassDeclaration& declaration);
    void parseClassHeader(ClassDeclaration& declaration);
    void parseClassBody  (ClassDeclaration& declaration);

    void parseClassMember(ClassDeclaration& declaration);
    void parseFunction   (ClassDeclaration& declaration, ValueType returnType, const Token& nameToken);

    void skipTerminators();
    void endStatement   ();
    void recover        ();

    [[noreturn]] void fail(const std::string& message);

    const Token& current() const;
    const Token& expect (TokenKind kind, const char* description);

    bool match(TokenKind kind);

    ValueType parseType();

    MemberDeclaration parseVariable(ValueType type, const Token& nameToken);

    std::vector<StatementPtr> parseBlock();

    StatementPtr parseStatement   ();
    StatementPtr parseDeclaration ();
    StatementPtr parseAssignOrCall();
    StatementPtr parseSimple      (StatementKind kind);
    StatementPtr parseReturn      ();

    StatementPtr parseIf   ();
    StatementPtr parseFor  ();
    StatementPtr parseWhile();

    StatementPtr makeStatement(StatementKind kind, const Token& token);

    ExpressionPtr parseExpression();
    ExpressionPtr parseBinary    (int minimumPrecedence);
    ExpressionPtr parseUnary     ();
    ExpressionPtr parsePostfix   ();
    ExpressionPtr parsePrimary   ();

    ExpressionPtr makeExpression(ExpressionKind kind, const Token& token);

    static int precedenceOf(TokenKind kind);

    std::span<const Token>         tokens;
    std::vector<ScriptDiagnostic>& diagnostics;

    std::size_t position = 0;
};
