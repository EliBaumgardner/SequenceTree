#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "ScriptCompiler.h"

enum class TokenKind
{
    Null,
    Invalid,
    End,
    Terminator,
    Identifier,
    Number,

    KeywordLet,
    KeywordIf,
    KeywordElse,
    KeywordFor,
    KeywordIn,
    KeywordWhile,
    KeywordBreak,
    KeywordContinue,
    KeywordReturn,

    LeftBrace,
    RightBrace,
    LeftParen,
    RightParen,
    Dot,

    Assign,
    PlusAssign,
    MinusAssign,

    Plus,
    Minus,
    Star,
    Slash,
    Percent,

    EqualEqual,
    NotEqual,
    Less,
    LessOrEqual,
    Greater,
    GreaterOrEqual,

    And,
    Or,
    Not
};

struct Token
{
    TokenKind   kind   = TokenKind::Null;
    std::string text;
    int         value  = 0;
    int         line   = 1;
    int         column = 1;
    int         length = 1;
};

class Lexer
{
public:

    Lexer(const std::string& sourceText, std::vector<ScriptDiagnostic>& diagnosticList)
        : source(sourceText), diagnostics(diagnosticList) {}

    std::vector<Token> run();

private:
    static bool isIdentifierPart(char c);

    char peek(std::size_t offset) const;

    void advance(int count = 1);

    Token makeToken(TokenKind kind, int length) const;

    Token readNumber();
    Token readIdentifier();
    Token readPunctuation();
    Token readFixed(TokenKind kind, int length);

    void report(const std::string& message, const Token& token);

    const std::string&             source;
    std::vector<ScriptDiagnostic>& diagnostics;

    std::size_t position = 0;
    int         line     = 1;
    int         column   = 1;
};
