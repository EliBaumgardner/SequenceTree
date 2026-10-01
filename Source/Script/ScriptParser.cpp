#include "ScriptParser.h"

ClassDeclaration Parser::run()
{
    ClassDeclaration declaration;

    skipTerminators();

    try {
        parseClassHeader(declaration);
    } catch (const ParseFailure&) {
        return declaration;
    }

    parseClassBody(declaration);

    skipTerminators();

    if (current().kind != TokenKind::End) {
        diagnostics.push_back({ "nothing may follow the class", current().line, current().column, current().length });
    }

    return declaration;
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
                return;
            }

            --depth;

            if (depth == 0) {
                ++position;
                return;
            }
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

ValueType Parser::parseType()
{
    if (match(TokenKind::KeywordInt)) {
        return ValueType::Int;
    }

    if (match(TokenKind::KeywordFloat)) {
        return ValueType::Float;
    }

    if (match(TokenKind::KeywordDouble)) {
        return ValueType::Double;
    }

    if (match(TokenKind::KeywordNode)) {
        return ValueType::Node;
    }

    fail("expected a type, 'int', 'float', 'double' or 'Node'");
}

void Parser::parseClassHeader(ClassDeclaration& declaration)
{
    if (current().kind != TokenKind::KeywordClass) {
        fail("a traversal script is a class, such as 'class Traversal : defaultTraversal { ... };'");
    }

    ++position;

    declaration.name = expect(TokenKind::Identifier, "a class name").text;

    if (match(TokenKind::Colon)) {
        const Token& base = expect(TokenKind::Identifier, "'defaultTraversal' after ':'");

        if (base.text != "defaultTraversal") {
            diagnostics.push_back({ "a traversal can only build on 'defaultTraversal'",
                                    base.line, base.column, base.length });
            throw ParseFailure{};
        }
    }

    expect(TokenKind::LeftBrace, "'{' to open the class");
}

void Parser::parseClassBody(ClassDeclaration& declaration)
{
    skipTerminators();

    while (current().kind != TokenKind::RightBrace) {
        if (current().kind == TokenKind::End) {
            diagnostics.push_back({ "expected '}' to close the class",
                                    current().line, current().column, current().length });
            return;
        }

        try {
            parseClassMember(declaration);
        } catch (const ParseFailure&) {
            recover();
        }

        skipTerminators();
    }

    ++position;
}

void Parser::parseClassMember(ClassDeclaration& declaration)
{
    ValueType type = ValueType::Void;

    if (match(TokenKind::KeywordPublic)) {
        expect(TokenKind::Colon, "':' after 'public'");
        return;
    }

    if (current().kind == TokenKind::Identifier) {
        fail("give '" + current().text + "' a type, such as 'Node " + current().text + ";' or 'int "
             + current().text + ";'");
    }

    if (!match(TokenKind::KeywordVoid)) {
        type = parseType();
    }

    const Token& nameToken = expect(TokenKind::Identifier, "a name");

    if (current().kind == TokenKind::LeftParen) {
        parseFunction(declaration, type, nameToken);
        return;
    }

    if (type == ValueType::Void) {
        fail("only a function can be 'void'");
    }

    if (current().kind == TokenKind::Assign) {
        fail("a member starts at 0, or at none for a Node; give it a value inside a function");
    }

    expect(TokenKind::Terminator, "';' after a member");

    declaration.members.push_back({ type, nameToken.text, nameToken.line, nameToken.column, nameToken.length });
}

void Parser::parseFunction(ClassDeclaration& declaration, ValueType returnType, const Token& nameToken)
{
    FunctionDeclaration function;

    function.returnType = returnType;
    function.name       = nameToken.text;
    function.line       = nameToken.line;
    function.column     = nameToken.column;
    function.length     = nameToken.length;

    expect(TokenKind::LeftParen, "'('");

    if (current().kind != TokenKind::RightParen) {
        do {
            Parameter parameter;

            parameter.line   = current().line;
            parameter.column = current().column;
            parameter.type   = parseType();
            parameter.name   = expect(TokenKind::Identifier, "a parameter name").text;
            parameter.length = tokens[position - 1].column + tokens[position - 1].length - parameter.column;

            function.parameters.push_back(parameter);
        } while (match(TokenKind::Comma));
    }

    expect(TokenKind::RightParen, "')' after the parameters");

    function.body = parseBlock();

    declaration.functions.push_back(std::move(function));
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
        case TokenKind::KeywordLet:
        case TokenKind::KeywordInt:
        case TokenKind::KeywordFloat:
        case TokenKind::KeywordDouble:
        case TokenKind::KeywordNode:     return parseDeclaration();
        case TokenKind::KeywordIf:       return parseIf();
        case TokenKind::KeywordFor:      return parseFor();
        case TokenKind::KeywordWhile:    return parseWhile();
        case TokenKind::KeywordBreak:    return parseSimple(StatementKind::Break);
        case TokenKind::KeywordContinue: return parseSimple(StatementKind::Continue);
        case TokenKind::KeywordReturn:   return parseReturn();
        case TokenKind::Identifier:      return parseAssignOrCall();
        default:                         break;
    }

    fail("expected a statement");
}

StatementPtr Parser::parseDeclaration()
{
    StatementPtr statement = makeStatement(StatementKind::Declare, current());

    if (match(TokenKind::KeywordLet)) {
        statement->declaredType = ValueType::Inferred;
    }
    else {
        statement->declaredType = parseType();
    }

    statement->name = expect(TokenKind::Identifier, "a variable name").text;

    if (statement->declaredType == ValueType::Inferred) {
        expect(TokenKind::Assign, "'=' after a variable name");
        statement->value = parseExpression();
    }
    else if (match(TokenKind::Assign)) {
        statement->value = parseExpression();
    }

    endStatement();

    return statement;
}

StatementPtr Parser::parseAssignOrCall()
{
    StatementPtr statement = makeStatement(StatementKind::Assign, current());

    ExpressionPtr   target     = parsePostfix();
    const TokenKind assignKind = current().kind;

    if (assignKind != TokenKind::Assign
        && assignKind != TokenKind::PlusAssign
        && assignKind != TokenKind::MinusAssign) {
        if (target->kind != ExpressionKind::Call) {
            fail("expected '=', '+=' or '-='");
        }

        statement->kind  = StatementKind::Call;
        statement->value = std::move(target);

        endStatement();

        return statement;
    }

    if (target->kind != ExpressionKind::Name && target->kind != ExpressionKind::Field) {
        fail("only a variable or a node property can be assigned");
    }

    statement->op     = assignKind;
    statement->target = std::move(target);
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

    ExpressionPtr iterable = parsePostfix();

    if (iterable->kind == ExpressionKind::Field && iterable->name == "children") {
        statement->target = std::move(iterable->left);
    }
    else if (iterable->kind != ExpressionKind::Name || iterable->name != "children") {
        diagnostics.push_back({ "only children can be iterated, such as 'children' or 'node.children'",
                                iterable->line, iterable->column, iterable->length });
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

    if (current().kind != TokenKind::Terminator) {
        statement->value = parseExpression();
    }

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

        try {
            body.push_back(parseStatement());
        } catch (const ParseFailure&) {
            recover();
        }

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
        case TokenKind::Or:             return 1;
        case TokenKind::And:            return 2;
        case TokenKind::EqualEqual:
        case TokenKind::NotEqual:       return 3;
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
    if (current().kind == TokenKind::Not || current().kind == TokenKind::Minus) {
        const Token& operatorToken = current();
        ++position;

        ExpressionPtr operand = parseUnary();

        if (operatorToken.kind == TokenKind::Minus
            && (operand->kind == ExpressionKind::Literal || operand->kind == ExpressionKind::Decimal)) {
            operand->value        = -operand->value;
            operand->decimalValue = -operand->decimalValue;
            return operand;
        }

        ExpressionPtr expression = makeExpression(ExpressionKind::Unary, operatorToken);

        expression->op   = operatorToken.kind;
        expression->left = std::move(operand);

        return expression;
    }

    return parsePostfix();
}

ExpressionPtr Parser::parsePostfix()
{
    ExpressionPtr expression = parsePrimary();

    while (match(TokenKind::Dot)) {
        const Token& memberToken = expect(TokenKind::Identifier, "a property name");

        ExpressionPtr field = makeExpression(ExpressionKind::Field, memberToken);

        field->name   = memberToken.text;
        field->line   = expression->line;
        field->column = expression->column;
        field->length = expression->length;

        if (memberToken.line == expression->line) {
            field->length = memberToken.column + memberToken.length - expression->column;
        }

        field->left = std::move(expression);
        expression  = std::move(field);
    }

    return expression;
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

    if (token.kind == TokenKind::Decimal) {
        ExpressionPtr expression = makeExpression(ExpressionKind::Decimal, token);
        expression->decimalValue = token.decimalValue;
        ++position;
        return expression;
    }

    if (token.kind == TokenKind::LeftParen) {
        ++position;

        ExpressionPtr expression = parseExpression();

        expect(TokenKind::RightParen, "')'");

        return expression;
    }

    if (token.kind == TokenKind::KeywordNone) {
        ++position;
        return makeExpression(ExpressionKind::None, token);
    }

    if (token.kind == TokenKind::Identifier) {
        ++position;

        if (match(TokenKind::LeftParen)) {
            ExpressionPtr expression = makeExpression(ExpressionKind::Call, token);
            expression->name = token.text;

            if (current().kind != TokenKind::RightParen) {
                do {
                    expression->arguments.push_back(parseExpression());
                } while (match(TokenKind::Comma));
            }

            const Token& closing = expect(TokenKind::RightParen, "')' after the arguments");

            if (closing.line == token.line) {
                expression->length = closing.column + closing.length - token.column;
            }

            return expression;
        }

        ExpressionPtr expression = makeExpression(ExpressionKind::Name, token);
        expression->name = token.text;

        return expression;
    }

    fail("expected a value");
}
