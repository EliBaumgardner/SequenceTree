#include "ScriptLexer.h"

#include <cctype>

namespace script {

namespace {

struct KeywordEntry
{
    const char* name;
    TokenKind   kind;
};

const KeywordEntry keywordTable[] = {
    { "let",      TokenKind::KeywordLet },
    { "if",       TokenKind::KeywordIf },
    { "else",     TokenKind::KeywordElse },
    { "for",      TokenKind::KeywordFor },
    { "in",       TokenKind::KeywordIn },
    { "while",    TokenKind::KeywordWhile },
    { "break",    TokenKind::KeywordBreak },
    { "continue", TokenKind::KeywordContinue },
    { "return",   TokenKind::KeywordReturn },
    { "none",     TokenKind::KeywordNone },
    { "and",      TokenKind::AmpAmp },
    { "or",       TokenKind::PipePipe },
    { "not",      TokenKind::Bang }
};

TokenKind keywordKind(const std::string& word)
{
    for (const KeywordEntry& entry : keywordTable) {
        if (word == entry.name) {
            return entry.kind;
        }
    }

    return TokenKind::Identifier;
}

}

std::vector<Token> Lexer::run()
{
    std::vector<Token> tokens;

    while (position < source.size()) {
        const char current = source[position];

        if (current == ' ' || current == '\t' || current == '\r' || current == '\n') {
            advance();
            continue;
        }

        if (current == '/' && peek(1) == '/') {
            advance(2);

            while (position < source.size() && source[position] != '\n') {
                if (source[position] == '/' && peek(1) == '/') {
                    advance(2);
                    break;
                }

                advance();
            }

            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(current)) != 0) {
            tokens.push_back(readNumber());
            continue;
        }

        if (isIdentifierStart(current)) {
            tokens.push_back(readIdentifier());
            continue;
        }

        const Token punctuation = readPunctuation();

        if (punctuation.kind == TokenKind::End) {
            return {};
        }

        tokens.push_back(punctuation);
    }

    Token end = makeToken(TokenKind::End, 0);
    tokens.push_back(end);

    return tokens;
}

bool Lexer::isIdentifierStart(char c)
{
    return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
}

bool Lexer::isIdentifierPart(char c)
{
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

char Lexer::peek(std::size_t offset) const
{
    const std::size_t index = position + offset;
    return index < source.size() ? source[index] : '\0';
}

void Lexer::advance(int count)
{
    for (int i = 0; i < count; ++i) {
        if (source[position] == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }

        ++position;
    }
}

Token Lexer::makeToken(TokenKind kind, int length) const
{
    Token token;
    token.kind   = kind;
    token.line   = line;
    token.column = column;
    token.length = length;
    return token;
}

Token Lexer::readNumber()
{
    Token token = makeToken(TokenKind::Number, 0);

    long long accumulated = 0;
    bool      overflowed  = false;

    while (position < source.size() && std::isdigit(static_cast<unsigned char>(source[position])) != 0) {
        accumulated = accumulated * 10 + (source[position] - '0');

        if (accumulated > 2147483647LL) {
            overflowed  = true;
            accumulated = 2147483647LL;
        }

        token.text.push_back(source[position]);
        advance();
    }

    token.length = static_cast<int>(token.text.size());
    token.value  = static_cast<int>(accumulated);

    if (overflowed) {
        report("number is too large", token);
    }

    return token;
}

Token Lexer::readIdentifier()
{
    Token token = makeToken(TokenKind::Identifier, 0);

    while (position < source.size() && isIdentifierPart(source[position])) {
        token.text.push_back(source[position]);
        advance();
    }

    token.length = static_cast<int>(token.text.size());
    token.kind   = keywordKind(token.text);

    return token;
}

Token Lexer::readPunctuation()
{
    const char current = source[position];
    const char next    = peek(1);

    if (current == '=' && next == '=') { return readFixed(TokenKind::EqualEqual, 2); }
    if (current == '!' && next == '=') { return readFixed(TokenKind::BangEqual, 2); }
    if (current == '<' && next == '=') { return readFixed(TokenKind::LessOrEqual, 2); }
    if (current == '>' && next == '=') { return readFixed(TokenKind::GreaterOrEqual, 2); }
    if (current == '&' && next == '&') { return readFixed(TokenKind::AmpAmp, 2); }
    if (current == '|' && next == '|') { return readFixed(TokenKind::PipePipe, 2); }
    if (current == '+' && next == '=') { return readFixed(TokenKind::PlusAssign, 2); }
    if (current == '-' && next == '=') { return readFixed(TokenKind::MinusAssign, 2); }

    switch (current) {
        case ';': return readFixed(TokenKind::Terminator, 1);
        case '{': return readFixed(TokenKind::LeftBrace, 1);
        case '}': return readFixed(TokenKind::RightBrace, 1);
        case '(': return readFixed(TokenKind::LeftParen, 1);
        case ')': return readFixed(TokenKind::RightParen, 1);
        case '.': return readFixed(TokenKind::Dot, 1);
        case '=': return readFixed(TokenKind::Assign, 1);
        case '+': return readFixed(TokenKind::Plus, 1);
        case '-': return readFixed(TokenKind::Minus, 1);
        case '*': return readFixed(TokenKind::Star, 1);
        case '/': return readFixed(TokenKind::Slash, 1);
        case '%': return readFixed(TokenKind::Percent, 1);
        case '<': return readFixed(TokenKind::Less, 1);
        case '>': return readFixed(TokenKind::Greater, 1);
        case '!': return readFixed(TokenKind::Bang, 1);
        default:  break;
    }

    Token unexpected = makeToken(TokenKind::End, 1);
    unexpected.text.push_back(current);

    report(std::string("unexpected character '") + current + "'", unexpected);

    return unexpected;
}

Token Lexer::readFixed(TokenKind kind, int length)
{
    Token token = makeToken(kind, length);

    for (int index = 0; index < length; ++index) {
        token.text.push_back(source[position]);
        advance();
    }

    return token;
}

void Lexer::report(const std::string& message, const Token& token)
{
    diagnostics.push_back({ message, token.line, token.column, token.length });
}

}
