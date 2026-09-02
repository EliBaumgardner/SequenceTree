#include "ScriptParser.h"

namespace script {

namespace {

struct ParseFailure {};

}

std::vector<StatementPtr> Parser::run()
{
    std::vector<StatementPtr> program;

    skipTerminators();

    while (current().kind != TokenKind::End) {
        try {
            program.push_back(parseStatement());
        } catch (const ParseFailure&) {
            recover();
        }

        skipTerminators();
    }

    return program;
}

const Token& Parser::current() const { return tokens[position]; }

bool Parser::match(TokenKind kind)
{
    if (current().kind != kind) {
        return false;
    }

    ++position;
    return true;
}

const Token& Parser::expect(TokenKind kind, const char* description)
{
    if (current().kind != kind) {
        fail(std::string("expected ") + description);
    }

    return tokens[position++];
}

void Parser::skipTerminators()
{
    while (current().kind == TokenKind::Terminator) {
        ++position;
    }
}

void Parser::endStatement()
{
    expect(TokenKind::Terminator, "';' after a statement");
}

void Parser::recover()
{
    int depth = 0;

    while (current().kind != TokenKind::End) {
        if (current().kind == TokenKind::LeftBrace) {
            ++depth;
        }

        if (current().kind == TokenKind::RightBrace) {
            if (depth == 0) {
                ++position;
                return;
            }

            --depth;
        }

        if (current().kind == TokenKind::Terminator && depth == 0) {
            ++position;
            return;
        }

        ++position;
    }
}

void Parser::fail(const std::string& message)
{
    diagnostics.push_back({ message, current().line, current().column, current().length });
    throw ParseFailure{};
}

StatementPtr Parser::makeStatement(StatementKind kind, const Token& token)
{
    StatementPtr statement = std::make_unique<Statement>();

    statement->kind   = kind;
    statement->line   = token.line;
    statement->column = token.column;
    statement->length = token.length;

    return statement;
}

StatementPtr Parser::parseStatement()
{
    switch (current().kind) {
        case TokenKind::KeywordLet:      return parseLet();
        case TokenKind::KeywordIf:       return parseIf();
        case TokenKind::KeywordFor:      return parseFor();
        case TokenKind::KeywordWhile:    return parseWhile();
        case TokenKind::KeywordBreak:    return parseSimple(StatementKind::Break);
        case TokenKind::KeywordContinue: return parseSimple(StatementKind::Continue);
        case TokenKind::KeywordReturn:   return parseReturn();
        case TokenKind::Identifier:      return parseAssign();
        default:                         break;
    }

    fail("expected a statement");
}

StatementPtr Parser::parseLet()
{
    StatementPtr statement = makeStatement(StatementKind::Let, current());
    ++position;

    statement->name = expect(TokenKind::Identifier, "a variable name").text;

    expect(TokenKind::Assign, "'=' after a variable name");

    statement->value = parseExpression();

    endStatement();

    return statement;
}

StatementPtr Parser::parseAssign()
{
    StatementPtr statement = makeStatement(StatementKind::Assign, current());

    statement->name = current().text;
    ++position;

    const TokenKind assignKind = current().kind;

    if (assignKind != TokenKind::Assign
        && assignKind != TokenKind::PlusAssign
        && assignKind != TokenKind::MinusAssign) {
        fail("expected '=', '+=' or '-=' after a variable name");
    }

    statement->op = assignKind;
    ++position;

    statement->value = parseExpression();

    endStatement();

    return statement;
}

StatementPtr Parser::parseIf()
{
    StatementPtr statement = makeStatement(StatementKind::If, current());
    ++position;

    statement->value = parseExpression();
    statement->body  = parseBlock();

    if (match(TokenKind::KeywordElse)) {
        statement->hasElse = true;

        if (current().kind == TokenKind::KeywordIf) {
            statement->elseBody.push_back(parseIf());
            return statement;
        }

        statement->elseBody = parseBlock();
    }

    return statement;
}

StatementPtr Parser::parseFor()
{
    StatementPtr statement = makeStatement(StatementKind::For, current());
    ++position;

    statement->name = expect(TokenKind::Identifier, "a loop variable name").text;

    expect(TokenKind::KeywordIn, "'in' after a loop variable name");

    const Token& iterable = expect(TokenKind::Identifier, "'children'");

    if (iterable.text != "children") {
        diagnostics.push_back({ "only 'children' can be iterated",
                                iterable.line, iterable.column, iterable.length });
        throw ParseFailure{};
    }

    statement->body = parseBlock();

    return statement;
}

StatementPtr Parser::parseWhile()
{
    StatementPtr statement = makeStatement(StatementKind::While, current());
    ++position;

    statement->value = parseExpression();
    statement->body  = parseBlock();

    return statement;
}

StatementPtr Parser::parseSimple(StatementKind kind)
{
    StatementPtr statement = makeStatement(kind, current());
    ++position;

    endStatement();

    return statement;
}

StatementPtr Parser::parseReturn()
{
    StatementPtr statement = makeStatement(StatementKind::Return, current());
    ++position;

    statement->value = parseExpression();

    endStatement();

    return statement;
}

std::vector<StatementPtr> Parser::parseBlock()
{
    expect(TokenKind::LeftBrace, "'{'");

    std::vector<StatementPtr> body;

    skipTerminators();

    while (current().kind != TokenKind::RightBrace) {
        if (current().kind == TokenKind::End) {
            fail("expected '}'");
        }

        body.push_back(parseStatement());

        skipTerminators();
    }

    ++position;

    return body;
}

ExpressionPtr Parser::makeExpression(ExpressionKind kind, const Token& token)
{
    ExpressionPtr expression = std::make_unique<Expression>();

    expression->kind   = kind;
    expression->line   = token.line;
    expression->column = token.column;
    expression->length = token.length;

    return expression;
}

int Parser::precedenceOf(TokenKind kind)
{
    switch (kind) {
        case TokenKind::PipePipe:       return 1;
        case TokenKind::AmpAmp:         return 2;
        case TokenKind::EqualEqual:
        case TokenKind::BangEqual:      return 3;
        case TokenKind::Less:
        case TokenKind::LessOrEqual:
        case TokenKind::Greater:
        case TokenKind::GreaterOrEqual: return 4;
        case TokenKind::Plus:
        case TokenKind::Minus:          return 5;
        case TokenKind::Star:
        case TokenKind::Slash:
        case TokenKind::Percent:        return 6;
        default:                        return 0;
    }
}

ExpressionPtr Parser::parseExpression() { return parseBinary(0); }

ExpressionPtr Parser::parseBinary(int minimumPrecedence)
{
    ExpressionPtr left = parseUnary();

    while (true) {
        const int precedence = precedenceOf(current().kind);

        if (precedence == 0 || precedence <= minimumPrecedence) {
            break;
        }

        const Token& operatorToken = current();
        ++position;

        ExpressionPtr right = parseBinary(precedence);

        ExpressionPtr combined = makeExpression(ExpressionKind::Binary, operatorToken);

        combined->op    = operatorToken.kind;
        combined->left  = std::move(left);
        combined->right = std::move(right);

        left = std::move(combined);
    }

    return left;
}

ExpressionPtr Parser::parseUnary()
{
    if (current().kind == TokenKind::Bang || current().kind == TokenKind::Minus) {
        const Token& operatorToken = current();
        ++position;

        ExpressionPtr operand = parseUnary();

        if (operatorToken.kind == TokenKind::Minus
            && operand->kind == ExpressionKind::Literal) {
            operand->value = -operand->value;
            return operand;
        }

        ExpressionPtr expression = makeExpression(ExpressionKind::Unary, operatorToken);

        expression->op   = operatorToken.kind;
        expression->left = std::move(operand);

        return expression;
    }

    return parsePrimary();
}

ExpressionPtr Parser::parsePrimary()
{
    const Token& token = current();

    if (token.kind == TokenKind::Number) {
        ExpressionPtr expression = makeExpression(ExpressionKind::Literal, token);
        expression->value = token.value;
        ++position;
        return expression;
    }

    if (token.kind == TokenKind::KeywordNone) {
        ExpressionPtr expression = makeExpression(ExpressionKind::Literal, token);
        expression->value = -1;
        ++position;
        return expression;
    }

    if (token.kind == TokenKind::LeftParen) {
        ++position;

        ExpressionPtr expression = parseExpression();

        expect(TokenKind::RightParen, "')'");

        return expression;
    }

    if (token.kind == TokenKind::Identifier) {
        ++position;

        if (current().kind == TokenKind::Dot) {
            ++position;

            const Token& memberToken = expect(TokenKind::Identifier, "a property name");

            ExpressionPtr expression = makeExpression(ExpressionKind::Member, token);

            expression->name   = token.text;
            expression->member = memberToken.text;
            expression->length = memberToken.column + memberToken.length - token.column;

            return expression;
        }

        ExpressionPtr expression = makeExpression(ExpressionKind::Name, token);
        expression->name = token.text;

        return expression;
    }

    fail("expected a value");
}

}
