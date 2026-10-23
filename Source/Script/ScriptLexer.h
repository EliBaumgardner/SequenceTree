#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "ScriptCompiler.h"

/*
    The first phase of the compiler: it splits script text into tokens, the words and symbols the
    Parser reads. A Token has a TokenKind (a keyword, name, number, bracket or operator), its text
    or value, and the line, column and length it came from, so errors can mark it. run drops
    whitespace and comments, which start at // and end at the next // or the end of the line, and
    ends the list with an End token. A character the language does not use, or a number too large,
    is reported as a ScriptDiagnostic. An unknown character stops the run with no tokens.

    A Lexer lives for one compile, made by compileTraversalScript. Parser also uses Token and
    TokenKind.
*/

enum class TokenKind
{
    Null,
    Invalid,
    End,
    Terminator,
    Identifier,
    Number,
    Decimal,

    KeywordLet,
    KeywordIf,
    KeywordElse,
    KeywordFor,
    KeywordIn,
    KeywordWhile,
    KeywordBreak,
    KeywordContinue,
    KeywordReturn,
    KeywordClass,
    KeywordPublic,
    KeywordVoid,
    KeywordInt,
    KeywordFloat,
    KeywordDouble,
    KeywordBool,
    KeywordNode,
    KeywordNone,
    KeywordTrue,
    KeywordFalse,
    KeywordImport,

    LeftBrace,
    RightBrace,
    LeftParen,
    RightParen,
    Dot,
    Colon,
    Scope,
    Comma,

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
    TokenKind   kind         = TokenKind::Null;
    std::string text;
    int         value        = 0;
    double      decimalValue = 0.0;
    int         line         = 1;
    int         column       = 1;
    int         length       = 1;
};

struct KeywordEntry
{
    const char* name;
    TokenKind   kind;
};

class Lexer
{
public:

    Lexer(const std::string& sourceText, std::vector<ScriptDiagnostic>& diagnosticList)
        : source(sourceText), diagnostics(diagnosticList) {}

    std::vector<Token> run();

private:

    void advance(int count = 1);
    void report (const std::string& message, const Token& token);

    char peek(std::size_t offset) const;

    Token readNumber     ();
    Token readIdentifier ();
    Token readPunctuation();

    Token readFixed(TokenKind kind, int length);
    Token makeToken(TokenKind kind, int length) const;

    static bool isIdentifierPart(char c);

    const std::string&             source;
    std::vector<ScriptDiagnostic>& diagnostics;

    std::size_t position = 0;
    int         line     = 1;
    int         column   = 1;
};
