#include "ScriptEmitter.h"

#include <cstddef>

namespace script {

namespace {

struct FieldEntry
{
    const char* name;
    ScriptField field;
};

const FieldEntry childFieldTable[] = {
    { "id",           ScriptField::ChildId },
    { "eligible",     ScriptField::ChildIsEligible },
    { "limit",        ScriptField::ChildCountLimit },
    { "countLimit",   ScriptField::ChildCountLimit },
    { "triggerLimit", ScriptField::ChildTriggerLimit },
    { "triggerCount", ScriptField::ChildTriggerCount },
    { "visits",       ScriptField::ChildVisitCount },
    { "visitCount",   ScriptField::ChildVisitCount },
    { "repeat",       ScriptField::ChildRepeatValue },
    { "pitch",        ScriptField::ChildPitchOffset },
    { "pitchOffset",  ScriptField::ChildPitchOffset },
    { "switchLimit",  ScriptField::ChildSwitchCountLimit },
    { "subLoopLimit", ScriptField::ChildSubLoopCountLimit },
    { "probability",  ScriptField::ChildProbability }
};

const FieldEntry parentFieldTable[] = {
    { "id",         ScriptField::ParentId },
    { "count",      ScriptField::ParentCount },
    { "childCount", ScriptField::ParentChildCount },
    { "lastChild",  ScriptField::ParentLastChosenChild }
};

const FieldEntry childrenFieldTable[] = {
    { "count", ScriptField::ParentChildCount }
};

const FieldEntry traversalFieldTable[] = {
    { "id",     ScriptField::TraversalId },
    { "random", ScriptField::TraversalRandom }
};

template <std::size_t count>
bool lookupField(const FieldEntry (&table)[count], const std::string& name, ScriptField& field)
{
    for (const FieldEntry& entry : table) {
        if (name == entry.name) {
            field = entry.field;
            return true;
        }
    }

    return false;
}

struct EmitFailure {};

}

void Emitter::run(const std::vector<StatementPtr>& program)
{
    emitSequence(program);

    emit(ScriptOpcode::PushInt, -1);
    emit(ScriptOpcode::Return);

    script.localCount = highWaterSlot;

    if (highWaterStack > RTScript::maxStack) {
        diagnostics.push_back({ "expression nesting is too deep for the traversal stack", 1, 1, 1 });
    }

    if (highWaterSlot > RTScript::maxLocals) {
        diagnostics.push_back({ "too many variables for the traversal stack", 1, 1, 1 });
    }
}

int Emitter::stackDelta(ScriptOpcode opcode)
{
    switch (opcode) {
        case ScriptOpcode::PushInt:
        case ScriptOpcode::PushLocal:
        case ScriptOpcode::PushField:
            return 1;

        case ScriptOpcode::StoreLocal:
        case ScriptOpcode::Pop:
        case ScriptOpcode::LoadChild:
        case ScriptOpcode::JumpIfFalse:
        case ScriptOpcode::JumpIfTrue:
        case ScriptOpcode::Return:
            return -1;

        case ScriptOpcode::Halt:
        case ScriptOpcode::Negate:
        case ScriptOpcode::LogicalNot:
        case ScriptOpcode::Jump:
            return 0;

        default:
            return -1;
    }
}

int Emitter::emit(ScriptOpcode opcode, int operand)
{
    const int index = static_cast<int>(script.instructions.size());

    script.instructions.push_back({ opcode, operand });

    stackDepth += stackDelta(opcode);

    if (stackDepth > highWaterStack) {
        highWaterStack = stackDepth;
    }

    return index;
}

int Emitter::emit(ScriptOpcode opcode, ScriptField field)
{
    return emit(opcode, static_cast<int>(field));
}

int Emitter::here() const { return static_cast<int>(script.instructions.size()); }

void Emitter::patch(int jumpIndex, int target)
{
    script.instructions[static_cast<std::size_t>(jumpIndex)].operand = target;
}

void Emitter::fail(const std::string& message, const Statement& statement)
{
    diagnostics.push_back({ message, statement.line, statement.column, statement.length });
    throw EmitFailure{};
}

void Emitter::fail(const std::string& message, const Expression& expression)
{
    diagnostics.push_back({ message, expression.line, expression.column, expression.length });
    throw EmitFailure{};
}

int Emitter::allocateSlot()
{
    const int slot = nextSlot++;

    if (nextSlot > highWaterSlot) {
        highWaterSlot = nextSlot;
    }

    return slot;
}

int Emitter::declareLocal(const std::string& name)
{
    const int slot = allocateSlot();
    locals.push_back({ name, slot });
    return slot;
}

bool Emitter::findLocal(const std::string& name, int& slot) const
{
    for (std::size_t index = locals.size(); index > 0; --index) {
        if (locals[index - 1].name == name) {
            slot = locals[index - 1].slot;
            return true;
        }
    }

    return false;
}

void Emitter::openScope()
{
    scopeMarks.push_back(locals.size());
    slotMarks.push_back(nextSlot);
}

void Emitter::closeScope()
{
    locals.resize(scopeMarks.back());
    nextSlot = slotMarks.back();

    scopeMarks.pop_back();
    slotMarks.pop_back();
}

void Emitter::emitSequence(const std::vector<StatementPtr>& statements)
{
    for (const StatementPtr& statement : statements) {
        try {
            emitStatement(*statement);
        } catch (const EmitFailure&) {
            stackDepth = 0;
        }
    }
}

void Emitter::emitBlock(const std::vector<StatementPtr>& body)
{
    openScope();

    emitSequence(body);

    closeScope();
}

void Emitter::emitStatement(const Statement& statement)
{
    switch (statement.kind) {
        case StatementKind::Let:      emitLet(statement);      break;
        case StatementKind::Assign:   emitAssign(statement);   break;
        case StatementKind::If:       emitIf(statement);       break;
        case StatementKind::For:      emitFor(statement);      break;
        case StatementKind::While:    emitWhile(statement);    break;
        case StatementKind::Break:    emitBreak(statement);    break;
        case StatementKind::Continue: emitContinue(statement); break;
        case StatementKind::Return:   emitReturn(statement);   break;
    }
}

void Emitter::emitLet(const Statement& statement)
{
    emitExpression(*statement.value);

    if (statement.name == childBinding) {
        fail("'" + statement.name + "' is the loop variable and cannot be redeclared", statement);
    }

    emit(ScriptOpcode::StoreLocal, declareLocal(statement.name));
}

void Emitter::emitAssign(const Statement& statement)
{
    int slot = 0;

    if (!findLocal(statement.name, slot)) {
        if (statement.name == childBinding) {
            fail("'" + statement.name + "' is the loop variable and cannot be assigned", statement);
        }

        fail("'" + statement.name + "' is not declared; use 'let " + statement.name + " = ...'", statement);
    }

    if (statement.op != TokenKind::Assign) {
        emit(ScriptOpcode::PushLocal, slot);
    }

    emitExpression(*statement.value);

    if (statement.op == TokenKind::PlusAssign) {
        emit(ScriptOpcode::Add);
    }

    if (statement.op == TokenKind::MinusAssign) {
        emit(ScriptOpcode::Subtract);
    }

    emit(ScriptOpcode::StoreLocal, slot);
}

void Emitter::emitIf(const Statement& statement)
{
    emitExpression(*statement.value);

    const int falseJump = emit(ScriptOpcode::JumpIfFalse);

    emitBlock(statement.body);

    if (!statement.hasElse) {
        patch(falseJump, here());
        return;
    }

    const int endJump = emit(ScriptOpcode::Jump);

    patch(falseJump, here());

    emitBlock(statement.elseBody);

    patch(endJump, here());
}

void Emitter::emitFor(const Statement& statement)
{
    if (!childBinding.empty()) {
        fail("child loops cannot be nested", statement);
    }

    openScope();

    const int indexSlot = allocateSlot();

    emit(ScriptOpcode::PushInt, 0);
    emit(ScriptOpcode::StoreLocal, indexSlot);

    const int loopTop = here();

    emit(ScriptOpcode::PushLocal, indexSlot);
    emit(ScriptOpcode::PushField, ScriptField::ParentChildCount);
    emit(ScriptOpcode::Less);

    const int exitJump = emit(ScriptOpcode::JumpIfFalse);

    emit(ScriptOpcode::PushLocal, indexSlot);
    emit(ScriptOpcode::LoadChild);

    childBinding = statement.name;
    loopStack.push_back({});

    emitBlock(statement.body);

    LoopFrame frame = std::move(loopStack.back());
    loopStack.pop_back();
    childBinding.clear();

    const int continueTarget = here();

    emit(ScriptOpcode::PushLocal, indexSlot);
    emit(ScriptOpcode::PushInt, 1);
    emit(ScriptOpcode::Add);
    emit(ScriptOpcode::StoreLocal, indexSlot);
    emit(ScriptOpcode::Jump, loopTop);

    const int exitTarget = here();

    patch(exitJump, exitTarget);

    closeScope();

    patchLoopFrame(frame, continueTarget, exitTarget);
}

void Emitter::emitWhile(const Statement& statement)
{
    const int loopTop = here();

    emitExpression(*statement.value);

    const int exitJump = emit(ScriptOpcode::JumpIfFalse);

    loopStack.push_back({});

    emitBlock(statement.body);

    LoopFrame frame = std::move(loopStack.back());
    loopStack.pop_back();

    emit(ScriptOpcode::Jump, loopTop);

    const int exitTarget = here();

    patch(exitJump, exitTarget);

    patchLoopFrame(frame, loopTop, exitTarget);
}

void Emitter::patchLoopFrame(const LoopFrame& frame, int continueTarget, int exitTarget)
{
    for (const int jumpIndex : frame.continueJumps) {
        patch(jumpIndex, continueTarget);
    }

    for (const int jumpIndex : frame.breakJumps) {
        patch(jumpIndex, exitTarget);
    }
}

void Emitter::emitBreak(const Statement& statement)
{
    if (loopStack.empty()) {
        fail("'break' is only valid inside a loop", statement);
    }

    loopStack.back().breakJumps.push_back(emit(ScriptOpcode::Jump));
}

void Emitter::emitContinue(const Statement& statement)
{
    if (loopStack.empty()) {
        fail("'continue' is only valid inside a loop", statement);
    }

    loopStack.back().continueJumps.push_back(emit(ScriptOpcode::Jump));
}

void Emitter::emitReturn(const Statement& statement)
{
    emitExpression(*statement.value);
    emit(ScriptOpcode::Return);
}

void Emitter::emitExpression(const Expression& expression)
{
    switch (expression.kind) {
        case ExpressionKind::Literal: emitLiteral(expression); break;
        case ExpressionKind::Name:    emitName(expression);    break;
        case ExpressionKind::Member:  emitMember(expression);  break;
        case ExpressionKind::Unary:   emitUnary(expression);   break;
        case ExpressionKind::Binary:  emitBinary(expression);  break;
    }
}

void Emitter::emitLiteral(const Expression& expression)
{
    emit(ScriptOpcode::PushInt, expression.value);
}

void Emitter::emitName(const Expression& expression)
{
    int slot = 0;

    if (findLocal(expression.name, slot)) {
        emit(ScriptOpcode::PushLocal, slot);
        return;
    }

    if (!childBinding.empty() && expression.name == childBinding) {
        emit(ScriptOpcode::PushField, ScriptField::ChildId);
        return;
    }

    if (expression.name == "parent" || expression.name == "children"
        || expression.name == "traversal") {
        fail("'" + expression.name + "' needs a property, such as '" + expression.name + ".count'",
             expression);
    }

    fail("'" + expression.name + "' is not declared", expression);
}

void Emitter::emitMember(const Expression& expression)
{
    ScriptField field = ScriptField::ParentId;

    if (!childBinding.empty() && expression.name == childBinding) {
        if (!lookupField(childFieldTable, expression.member, field)) {
            fail("a child has no property '" + expression.member + "'", expression);
        }

        emit(ScriptOpcode::PushField, field);
        return;
    }

    if (expression.name == "parent") {
        if (!lookupField(parentFieldTable, expression.member, field)) {
            fail("the parent has no property '" + expression.member + "'", expression);
        }

        emit(ScriptOpcode::PushField, field);
        return;
    }

    if (expression.name == "children") {
        if (!lookupField(childrenFieldTable, expression.member, field)) {
            fail("'children' has no property '" + expression.member + "'", expression);
        }

        emit(ScriptOpcode::PushField, field);
        return;
    }

    if (expression.name == "traversal") {
        if (!lookupField(traversalFieldTable, expression.member, field)) {
            fail("the traversal has no property '" + expression.member + "'", expression);
        }

        emit(ScriptOpcode::PushField, field);
        return;
    }

    int slot = 0;

    if (findLocal(expression.name, slot)) {
        fail("'" + expression.name + "' is a number and has no properties", expression);
    }

    fail("'" + expression.name + "' is not declared; child properties are only available inside "
         "'for ... in children'", expression);
}

void Emitter::emitUnary(const Expression& expression)
{
    emitExpression(*expression.left);

    emit(expression.op == TokenKind::Minus ? ScriptOpcode::Negate : ScriptOpcode::LogicalNot);
}

void Emitter::emitBinary(const Expression& expression)
{
    emitExpression(*expression.left);
    emitExpression(*expression.right);

    emit(binaryOpcode(expression.op));
}

ScriptOpcode Emitter::binaryOpcode(TokenKind kind)
{
    switch (kind) {
        case TokenKind::Plus:           return ScriptOpcode::Add;
        case TokenKind::Minus:          return ScriptOpcode::Subtract;
        case TokenKind::Star:           return ScriptOpcode::Multiply;
        case TokenKind::Slash:          return ScriptOpcode::Divide;
        case TokenKind::Percent:        return ScriptOpcode::Modulo;
        case TokenKind::EqualEqual:     return ScriptOpcode::Equal;
        case TokenKind::BangEqual:      return ScriptOpcode::NotEqual;
        case TokenKind::Less:           return ScriptOpcode::Less;
        case TokenKind::LessOrEqual:    return ScriptOpcode::LessOrEqual;
        case TokenKind::Greater:        return ScriptOpcode::Greater;
        case TokenKind::GreaterOrEqual: return ScriptOpcode::GreaterOrEqual;
        case TokenKind::AmpAmp:         return ScriptOpcode::LogicalAnd;
        default:                        return ScriptOpcode::LogicalOr;
    }
}

}
