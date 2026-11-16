#pragma once

#include <cstddef>
#include <expected>
#include <span>
#include <string>
#include <vector>

#include "ScriptLexer.h"
#include "ScriptSyntaxTree.h"

/*
    The second phase of the compiler: it checks that the tokens follow the language's grammar and
    builds the ClassDeclaration they describe. run reads the imports, any variables before the
    class, the class with its members and functions, and any variables after it. Inside functions,
    parseStatement reads declarations, assignments, calls, if / else, for, while, break, continue
    and return, and parseExpression reads operators in order of precedence (precedenceOf).

    It reports a break in the grammar as a ScriptDiagnostic. Every function that reads the tokens
    returns a Parsed value: the thing it read, or a ParseFailure once the diagnostic is filed, and
    each caller passes a failure up. The loops over class members, global variables and statements
    stop the failure, call recover to skip to the end of the broken member or statement and carry
    on, so one compile can list several errors. It does not check that names exist or that types
    agree; the Emitter does. A Parser lives for one compile, made by compileTraversalScript.
*/

struct ParseFailure {};

template <typename Value>
using Parsed = std::expected<Value, ParseFailure>;

using ParseStatus = Parsed<void>;

class Parser
{
public:

    Parser(std::span<const Token> tokenList, std::vector<ScriptDiagnostic>& diagnosticList)
        : tokens     (tokenList),
          diagnostics(diagnosticList) {}

    ClassDeclaration run();

private:

    [[nodiscard]] ParseStatus parseImports    (ClassDeclaration& declaration);
    [[nodiscard]] ParseStatus parseGlobal     (ClassDeclaration& declaration);
    [[nodiscard]] ParseStatus parseClassHeader(ClassDeclaration& declaration);
    [[nodiscard]] ParseStatus parseClassMember(ClassDeclaration& declaration);
    [[nodiscard]] ParseStatus parseFunction   (ClassDeclaration& declaration, ValueType returnType, const Token& nameToken);
    [[nodiscard]] ParseStatus endStatement    ();

    void parseGlobals  (ClassDeclaration& declaration);
    void parseClassBody(ClassDeclaration& declaration);

    void skipTerminators();
    void recover        ();

    [[nodiscard]] std::unexpected<ParseFailure> fail(const std::string& message);

    const Token& current() const;

    [[nodiscard]] Parsed<const Token*> expect(TokenKind kind, const char* description);

    bool match(TokenKind kind);

    [[nodiscard]] Parsed<ValueType> parseType();

    [[nodiscard]] Parsed<MemberDeclaration> parseVariable(ValueType type, const Token& nameToken);

    [[nodiscard]] Parsed<std::vector<StatementPtr>> parseBlock();

    [[nodiscard]] Parsed<StatementPtr> parseStatement   ();
    [[nodiscard]] Parsed<StatementPtr> parseDeclaration ();
    [[nodiscard]] Parsed<StatementPtr> parseAssignOrCall();
    [[nodiscard]] Parsed<StatementPtr> parseSimple      (StatementKind kind);
    [[nodiscard]] Parsed<StatementPtr> parseReturn      ();

    [[nodiscard]] Parsed<StatementPtr> parseIf   ();
    [[nodiscard]] Parsed<StatementPtr> parseFor  ();
    [[nodiscard]] Parsed<StatementPtr> parseWhile();

    StatementPtr makeStatement(StatementKind kind, const Token& token);

    [[nodiscard]] Parsed<ExpressionPtr> parseExpression();
    [[nodiscard]] Parsed<ExpressionPtr> parseBinary    (int minimumPrecedence);
    [[nodiscard]] Parsed<ExpressionPtr> parseUnary     ();
    [[nodiscard]] Parsed<ExpressionPtr> parsePostfix   ();
    [[nodiscard]] Parsed<ExpressionPtr> parsePrimary   ();

    ExpressionPtr makeExpression(ExpressionKind kind, const Token& token);

    static int precedenceOf(TokenKind kind);

    std::span<const Token>         tokens;
    std::vector<ScriptDiagnostic>& diagnostics;

    SyntaxArena* arena = nullptr;

    std::size_t position = 0;
};
