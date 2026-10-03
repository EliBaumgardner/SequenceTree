#include "ScriptEmitter.h"

#include <algorithm>
#include <cstddef>

void Emitter::run(const ClassDeclaration& declaration)
{
    imports = declaration.imports;

    declareMembers(declaration);
    declareFunctions(declaration);

    for (const FunctionSignature& signature : functions) {
        emitFunction(declaration.functions[static_cast<std::size_t>(signature.declaration)], signature);
    }
}

void Emitter::declareMembers(const ClassDeclaration& declaration)
{
    for (const MemberDeclaration& member : declaration.members) {
        int defaultValue = 0;

        if (findMember(member.name) != nullptr) {
            diagnostics.push_back({ "'" + member.name + "' is already a member",
                                    member.line, member.column, member.length });
            continue;
        }

        if (member.name == "current") {
            diagnostics.push_back({ "'current' is the node the walk is on and cannot be a member",
                                    member.line, member.column, member.length });
            continue;
        }

        if (static_cast<int>(members.size()) >= RTScript::maxMembers) {
            diagnostics.push_back({ "too many members for the traversal",
                                    member.line, member.column, member.length });
            continue;
        }

        if (member.type == ValueType::Node) {
            defaultValue = -1;
        }

        members.push_back({ member.name, static_cast<int>(members.size()), member.type });
        script.memberDefaults.push_back(defaultValue);
    }
}

const LocalBinding* Emitter::findMember(const std::string& name) const
{
    for (const LocalBinding& member : members) {
        if (member.name == name) {
            return &member;
        }
    }

    return nullptr;
}

void Emitter::declareFunctions(const ClassDeclaration& declaration)
{
    for (std::size_t index = 0; index < declaration.functions.size(); ++index) {
        const FunctionDeclaration& function = declaration.functions[index];

        const int functionIndex = static_cast<int>(functions.size());

        const bool isMain    = function.name == "main";
        const bool isAdvance = function.name == "advance";

        const bool mainMatches    = function.returnType == ValueType::Void && function.parameters.empty();
        const bool advanceMatches = function.returnType == ValueType::Node && function.parameters.size() == 1
                                    && function.parameters.front().type == ValueType::Int;

        FunctionSignature signature { function.name, function.returnType, {}, functionIndex,
                                      static_cast<int>(index) };

        if (findFunction(function.name) != nullptr) {
            diagnostics.push_back({ "'" + function.name + "' is already a function",
                                    function.line, function.column, function.length });
            continue;
        }

        if (function.name == "playNote") {
            diagnostics.push_back({ "'playNote' is built in and cannot be redefined",
                                    function.line, function.column, function.length });
            continue;
        }

        if (isMain && !mainMatches) {
            diagnostics.push_back({ "main is written 'void main()'",
                                    function.line, function.column, function.length });
            continue;
        }

        if (isAdvance && !advanceMatches) {
            diagnostics.push_back({ "advance is written 'Node advance(int numSteps)'",
                                    function.line, function.column, function.length });
            continue;
        }

        if (isMain) {
            script.mainFunction = functionIndex;
        }

        if (isAdvance) {
            script.advanceFunction = functionIndex;
        }

        for (const Parameter& parameter : function.parameters) {
            signature.parameterTypes.push_back(parameter.type);
        }

        functions.push_back(signature);
    }

    script.functions.resize(functions.size());
}

const FunctionSignature* Emitter::findFunction(const std::string& name) const
{
    for (const FunctionSignature& signature : functions) {
        if (signature.name == name) {
            return &signature;
        }
    }

    return nullptr;
}

void Emitter::emitFunction(const FunctionDeclaration& declaration, const FunctionSignature& signature)
{
    ScriptFunction& function = script.functions[static_cast<std::size_t>(signature.index)];

    int defaultReturn = 0;

    locals.clear();
    scopeMarks.clear();
    slotMarks.clear();
    loopStack.clear();

    nextSlot        = 0;
    highWaterSlot   = 0;
    stackDepth      = 0;
    highWaterStack  = 0;
    currentFunction = &signature;

    function.entry          = here();
    function.parameterCount = static_cast<int>(declaration.parameters.size());

    for (const Parameter& parameter : declaration.parameters) {
        if (findLocal(parameter.name) != nullptr) {
            diagnostics.push_back({ "'" + parameter.name + "' is already a parameter",
                                    parameter.line, parameter.column, parameter.length });
        }

        declareLocal(parameter.name, parameter.type);
    }

    emitSequence(declaration.body);

    if (signature.returnType == ValueType::Node) {
        defaultReturn = -1;
    }

    emit(ScriptOpcode::PushInt, defaultReturn);
    emit(ScriptOpcode::Return);

    function.localCount = highWaterSlot;

    if (highWaterStack > RTScript::maxStack) {
        diagnostics.push_back({ "expression nesting is too deep for the traversal stack",
                                declaration.line, declaration.column, declaration.length });
    }

    if (highWaterSlot > RTScript::maxLocals) {
        diagnostics.push_back({ "too many variables for the traversal stack",
                                declaration.line, declaration.column, declaration.length });
    }

    currentFunction = nullptr;
}

int Emitter::here() const { return static_cast<int>(script.instructions.size()); }

const LocalBinding* Emitter::findLocal(const std::string& name) const
{
    for (std::size_t index = locals.size(); index > 0; --index) {
        if (locals[index - 1].name == name) {
            return &locals[index - 1];
        }
    }

    return nullptr;
}

int Emitter::declareLocal(const std::string& name, ValueType type)
{
    const int slot = allocateSlot();
    locals.push_back({ name, slot, type });
    return slot;
}

int Emitter::allocateSlot()
{
    const int slot = nextSlot++;

    if (nextSlot > highWaterSlot) {
        highWaterSlot = nextSlot;
    }

    return slot;
}

void Emitter::emitSequence(std::span<const StatementPtr> statements)
{
    for (const StatementPtr& statement : statements) {
        const std::size_t scopeDepth = scopeMarks.size();
        const std::size_t localCount = locals.size();
        const int         slotMark   = nextSlot;

        try {
            emitStatement(*statement);
        } catch (const EmitFailure&) {
            scopeMarks.resize(scopeDepth);
            slotMarks.resize(scopeDepth);
            locals.resize(localCount);

            nextSlot   = slotMark;
            stackDepth = 0;
        }
    }
}

void Emitter::emitStatement(const Statement& statement)
{
    switch (statement.kind) {
        case StatementKind::Declare:  emitDeclare(statement);  break;
        case StatementKind::Assign:   emitAssign(statement);   break;
        case StatementKind::If:       emitIf(statement);       break;
        case StatementKind::For:      emitFor(statement);      break;
        case StatementKind::While:    emitWhile(statement);    break;
        case StatementKind::Break:    emitBreak(statement);    break;
        case StatementKind::Continue: emitContinue(statement); break;
        case StatementKind::Return:   emitReturn(statement);   break;

        case StatementKind::Call: {
            emitExpression(*statement.value);
            emit(ScriptOpcode::Pop);
            break;
        }
    }
}

void Emitter::emitDeclare(const Statement& statement)
{
    ValueType type         = statement.declaredType;
    int       defaultValue = 0;

    if (statement.value == nullptr) {
        if (type == ValueType::Node) {
            defaultValue = -1;
        }

        emit(ScriptOpcode::PushInt, defaultValue);
    }
    else {
        const ValueType valueType = emitExpression(*statement.value);

        if (valueType == ValueType::Void) {
            fail("this gives no value to store in '" + statement.name + "'", *statement.value);
        }

        if (type == ValueType::Inferred) {
            type = valueType;
        }

        if (!emitConversion(valueType, type)) {
            fail("'" + statement.name + "' holds " + typeName(type) + ", but this is " + typeName(valueType), *statement.value);
        }
    }

    emit(ScriptOpcode::StoreLocal, declareLocal(statement.name, type));
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

int Emitter::stackDelta(ScriptOpcode opcode)
{
    switch (opcode) {
        case ScriptOpcode::PushInt:
        case ScriptOpcode::PushReal:
        case ScriptOpcode::PushLocal:
        case ScriptOpcode::PushMember:
        case ScriptOpcode::PushContext:
        case ScriptOpcode::Call:
            return 1;

        case ScriptOpcode::Halt:
        case ScriptOpcode::PushNodeField:
        case ScriptOpcode::Negate:
        case ScriptOpcode::Convert:
        case ScriptOpcode::LogicalNot:
        case ScriptOpcode::Jump:
        case ScriptOpcode::Advance:
            return 0;

        case ScriptOpcode::StoreNodeField:
            return -2;

        case ScriptOpcode::PlayNote:
            return -3;

        default:
            return -1;
    }
}

ValueType Emitter::emitExpression(const Expression& expression)
{
    switch (expression.kind) {
        case ExpressionKind::Literal: {
            emit(ScriptOpcode::PushInt, expression.value);
            return ValueType::Int;
        }

        case ExpressionKind::Decimal: {
            emit(ScriptOpcode::PushReal, static_cast<int>(script.constants.size()));
            script.constants.push_back(expression.decimalValue);
            return ValueType::Double;
        }

        case ExpressionKind::None: {
            emit(ScriptOpcode::PushInt, -1);
            return ValueType::Node;
        }

        case ExpressionKind::Name:   return emitName(expression);
        case ExpressionKind::Field:  return emitField(expression);
        case ExpressionKind::Call:   return emitCall(expression);
        case ExpressionKind::Unary:  return emitUnary(expression);
        case ExpressionKind::Binary: return emitBinary(expression);
    }

    fail("expected a value", expression);
}

ValueType Emitter::emitName(const Expression& expression)
{
    const LocalBinding* const local  = findLocal(expression.name);
    const LocalBinding* const member = findMember(expression.name);

    if (local != nullptr) {
        emit(ScriptOpcode::PushLocal, local->slot);
        return local->type;
    }

    if (member != nullptr) {
        emit(ScriptOpcode::PushMember, member->slot);
        return member->type;
    }

    if (expression.name == "current") {
        emit(ScriptOpcode::PushContext, static_cast<int>(ScriptContextValue::Current));
        return ValueType::Node;
    }

    if (expression.name == "children") {
        fail("'children' can only be walked, as in 'for child in children { }'", expression);
    }

    if (expression.name == "traversal") {
        fail("'traversal' needs a property, such as 'traversal.random'", expression);
    }

    fail("'" + expression.name + "' is not declared", expression);
}

void Emitter::fail(const std::string& message, const Expression& expression)
{
    diagnostics.push_back({ message, expression.line, expression.column, expression.length });
    throw EmitFailure{};
}

const ContextEntry traversalContextTable[] = {
    { "id",       ScriptContextValue::TraversalId },
    { "random",   ScriptContextValue::TraversalRandom },
    { "instance", ScriptContextValue::TraversalInstance }
};

ValueType Emitter::emitField(const Expression& expression)
{
    const Expression& object = *expression.left;

    const bool namesTraversal = object.kind == ExpressionKind::Name && object.name == "traversal"
                                && findLocal(object.name) == nullptr && findMember(object.name) == nullptr;

    if (namesTraversal) {
        for (const ContextEntry& entry : traversalContextTable) {
            if (expression.name == entry.name) {
                emit(ScriptOpcode::PushContext, static_cast<int>(entry.value));
                return ValueType::Int;
            }
        }

        fail("the traversal has no property '" + expression.name + "'", expression);
    }

    const FieldEntry& field = nodeField(expression);

    if (emitExpression(object) != ValueType::Node) {
        fail("only a Node has properties", object);
    }

    emit(ScriptOpcode::PushNodeField, static_cast<int>(field.field));

    return field.type;
}

const FieldEntry nodeFieldTable[] = {
    { "id",           ScriptField::Id,           ValueType::Int,  FieldAccess::ReadOnly },
    { "pitch",        ScriptField::Pitch,        ValueType::Int,  FieldAccess::ReadOnly },
    { "velocity",     ScriptField::Velocity,     ValueType::Int,  FieldAccess::ReadOnly },
    { "duration",     ScriptField::Duration,     ValueType::Int,  FieldAccess::ReadOnly },
    { "count",        ScriptField::Count,        ValueType::Int,  FieldAccess::Writable },
    { "switchCount",  ScriptField::SwitchCount,  ValueType::Int,  FieldAccess::Writable },
    { "triggerCount", ScriptField::TriggerCount, ValueType::Int,  FieldAccess::Writable },
    { "subLoopCount", ScriptField::SubLoopCount, ValueType::Int,  FieldAccess::Writable },
    { "limit",        ScriptField::CountLimit,   ValueType::Int,  FieldAccess::ReadOnly },
    { "countLimit",   ScriptField::CountLimit,   ValueType::Int,  FieldAccess::ReadOnly },
    { "triggerLimit", ScriptField::TriggerLimit, ValueType::Int,  FieldAccess::ReadOnly },
    { "switchLimit",  ScriptField::SwitchLimit,  ValueType::Int,  FieldAccess::ReadOnly },
    { "subLoopLimit", ScriptField::SubLoopLimit, ValueType::Int,  FieldAccess::ReadOnly },
    { "repeat",       ScriptField::Repeat,       ValueType::Int,  FieldAccess::ReadOnly },
    { "probability",  ScriptField::Probability,  ValueType::Int,  FieldAccess::ReadOnly },
    { "childCount",   ScriptField::ChildCount,   ValueType::Int,  FieldAccess::ReadOnly },
    { "lastChild",    ScriptField::LastChild,    ValueType::Node, FieldAccess::ReadOnly },
    { "parent",       ScriptField::Parent,       ValueType::Node, FieldAccess::ReadOnly },
    { "eligible",     ScriptField::Eligible,     ValueType::Int,  FieldAccess::ReadOnly }
};

const FieldEntry& Emitter::nodeField(const Expression& expression)
{
    for (const FieldEntry& entry : nodeFieldTable) {
        if (expression.name == entry.name) {
            return entry;
        }
    }

    if (expression.name == "children") {
        fail("'children' can only be walked, as in 'for child in node.children { }'", expression);
    }

    fail("a Node has no property '" + expression.name + "'", expression);
}

ValueType Emitter::emitCall(const Expression& expression)
{
    const FunctionSignature* const signature = findFunction(expression.name);

    const bool isScoped   = !expression.scope.empty();
    const bool isImported = std::find(imports.begin(), imports.end(), expression.scope) != imports.end();

    const bool isAdvance    = !isScoped && expression.name == "advance";
    const bool isPlayNote   = !isScoped && expression.name == "playNote";
    const bool isCoreRandom = isScoped && expression.scope == "core" && expression.name == "random";

    const bool insideAdvance = currentFunction != nullptr && currentFunction->name == "advance";

    std::vector<ValueType> parameterTypes;
    ValueType              returnType = ValueType::Void;

    if (isScoped && !isImported) {
        fail("add 'import " + expression.scope + ";' at the top of the script to use '" + expression.scope + "::" + expression.name + "'", expression);
    }

    if (isAdvance) {
        parameterTypes = { ValueType::Int };
        returnType     = ValueType::Node;
    }
    else if (isPlayNote) {
        parameterTypes = { ValueType::Node, ValueType::Int, ValueType::Int, ValueType::Int };
    }
    else if (isCoreRandom) {
        parameterTypes = { ValueType::Double, ValueType::Double };
        returnType     = ValueType::Double;
    }
    else if (isScoped) {
        fail("'" + expression.scope + "' has no function '" + expression.name + "'", expression);
    }
    else if (signature != nullptr) {
        parameterTypes = signature->parameterTypes;
        returnType     = signature->returnType;
    }
    else {
        fail("'" + expression.name + "' is not a function", expression);
    }

    if ((isAdvance || isPlayNote) && insideAdvance) {
        fail("advance only chooses where the walk goes; call '" + expression.name + "' from main", expression);
    }

    if (expression.arguments.size() != parameterTypes.size()) {
        fail("'" + expression.name + "' takes " + std::to_string(parameterTypes.size()) + " values", expression);
    }

    for (std::size_t index = 0; index < parameterTypes.size(); ++index) {
        const Expression& argument = *expression.arguments[index];

        if (!emitConversion(emitExpression(argument), parameterTypes[index])) {
            fail("this should be " + std::string(typeName(parameterTypes[index])), argument);
        }
    }

    if (isAdvance) {
        emit(ScriptOpcode::Advance);
    }
    else if (isPlayNote) {
        emit(ScriptOpcode::PlayNote);
    }
    else if (isCoreRandom) {
        emit(ScriptOpcode::CoreRandom);
    }
    else {
        emit(ScriptOpcode::Call, signature->index);
        stackDepth -= static_cast<int>(parameterTypes.size());
    }

    return returnType;
}

bool Emitter::emitConversion(ValueType from, ValueType to)
{
    if (from == to) {
        return true;
    }

    if (commonType(from, to) == ValueType::Void) {
        return false;
    }

    if (to != ValueType::Double) {
        emit(ScriptOpcode::Convert, static_cast<int>(numberKind(to)));
    }

    return true;
}

ValueType Emitter::commonType(ValueType left, ValueType right)
{
    const bool leftIsNumber  = left == ValueType::Int || left == ValueType::Float || left == ValueType::Double;
    const bool rightIsNumber = right == ValueType::Int || right == ValueType::Float || right == ValueType::Double;

    if (!leftIsNumber || !rightIsNumber) {
        return ValueType::Void;
    }

    if (left == ValueType::Double || right == ValueType::Double) {
        return ValueType::Double;
    }

    if (left == ValueType::Float || right == ValueType::Float) {
        return ValueType::Float;
    }

    return ValueType::Int;
}

ScriptNumber Emitter::numberKind(ValueType type)
{
    switch (type) {
        case ValueType::Float:  return ScriptNumber::Float;
        case ValueType::Double: return ScriptNumber::Double;
        default:                return ScriptNumber::Int;
    }
}

const char* Emitter::typeName(ValueType type)
{
    switch (type) {
        case ValueType::Int:    return "an int";
        case ValueType::Float:  return "a float";
        case ValueType::Double: return "a double";
        case ValueType::Node:   return "a Node";
        default:                return "no value";
    }
}

ValueType Emitter::emitUnary(const Expression& expression)
{
    const ValueType operandType = emitExpression(*expression.left);

    if (commonType(operandType, operandType) == ValueType::Void) {
        fail("this needs a number", *expression.left);
    }

    if (expression.op == TokenKind::Minus) {
        emit(ScriptOpcode::Negate, static_cast<int>(numberKind(operandType)));
        return operandType;
    }

    emit(ScriptOpcode::LogicalNot, static_cast<int>(numberKind(operandType)));

    return ValueType::Int;
}

ValueType Emitter::emitBinary(const Expression& expression)
{
    const bool comparesIdentity = expression.op == TokenKind::EqualEqual || expression.op == TokenKind::NotEqual;

    const bool producesNumber = expression.op == TokenKind::Plus || expression.op == TokenKind::Minus
                                || expression.op == TokenKind::Star || expression.op == TokenKind::Slash
                                || expression.op == TokenKind::Percent;

    const ValueType leftType   = emitExpression(*expression.left);
    const ValueType rightType  = emitExpression(*expression.right);
    const ValueType numberType = commonType(leftType, rightType);

    const bool comparesNodes = comparesIdentity && leftType == ValueType::Node && rightType == ValueType::Node;

    if (comparesIdentity && !comparesNodes && numberType == ValueType::Void) {
        fail("'==' and '!=' compare two numbers or two Nodes", expression);
    }

    if (!comparesIdentity && numberType == ValueType::Void) {
        fail("this operator works on numbers; read a number from a Node, such as 'node.count'", expression);
    }

    if (expression.op == TokenKind::Percent && numberType != ValueType::Int) {
        fail("'%' works on ints; store the value in an int first", expression);
    }

    emit(binaryOpcode(expression), static_cast<int>(numberKind(numberType)));

    if (producesNumber) {
        return numberType;
    }

    return ValueType::Int;
}

ScriptOpcode Emitter::binaryOpcode(const Expression& expression)
{
    switch (expression.op) {
        case TokenKind::Plus:           return ScriptOpcode::Add;
        case TokenKind::Minus:          return ScriptOpcode::Subtract;
        case TokenKind::Star:           return ScriptOpcode::Multiply;
        case TokenKind::Slash:          return ScriptOpcode::Divide;
        case TokenKind::Percent:        return ScriptOpcode::Modulo;
        case TokenKind::EqualEqual:     return ScriptOpcode::Equal;
        case TokenKind::NotEqual:       return ScriptOpcode::NotEqual;
        case TokenKind::Less:           return ScriptOpcode::Less;
        case TokenKind::LessOrEqual:    return ScriptOpcode::LessOrEqual;
        case TokenKind::Greater:        return ScriptOpcode::Greater;
        case TokenKind::GreaterOrEqual: return ScriptOpcode::GreaterOrEqual;
        case TokenKind::And:            return ScriptOpcode::LogicalAnd;
        case TokenKind::Or:             return ScriptOpcode::LogicalOr;
        default:                        break;
    }

    fail("this operator cannot be used between two values", expression);
}

void Emitter::emitAssign(const Statement& statement)
{
    const Expression& target = *statement.target;

    const LocalBinding* local  = nullptr;
    const LocalBinding* member = nullptr;

    ScriptOpcode loadOpcode  = ScriptOpcode::PushLocal;
    ScriptOpcode storeOpcode = ScriptOpcode::StoreLocal;
    int          slot        = 0;
    ValueType    type        = ValueType::Int;

    if (target.kind == ExpressionKind::Field) {
        emitAssignField(statement);
        return;
    }

    local  = findLocal(target.name);
    member = findMember(target.name);

    if (local != nullptr) {
        slot = local->slot;
        type = local->type;
    }
    else if (member != nullptr) {
        loadOpcode  = ScriptOpcode::PushMember;
        storeOpcode = ScriptOpcode::StoreMember;
        slot        = member->slot;
        type        = member->type;
    }
    else if (target.name == "current") {
        fail("'current' is where the walk is; move it by returning a node from advance", target);
    }
    else {
        fail("'" + target.name + "' is not declared; declare it with 'int " + target.name + " = ...;'", target);
    }

    if (statement.op != TokenKind::Assign) {
        if (commonType(type, type) == ValueType::Void) {
            fail("only a number can be added to or taken from", target);
        }

        emit(loadOpcode, slot);
    }

    ValueType valueType = emitExpression(*statement.value);

    if (statement.op != TokenKind::Assign) {
        valueType = commonType(type, valueType);

        if (valueType == ValueType::Void) {
            fail("only a number can be added to or taken from", *statement.value);
        }
    }

    if (statement.op == TokenKind::PlusAssign) {
        emit(ScriptOpcode::Add, static_cast<int>(numberKind(valueType)));
    }

    if (statement.op == TokenKind::MinusAssign) {
        emit(ScriptOpcode::Subtract, static_cast<int>(numberKind(valueType)));
    }

    if (!emitConversion(valueType, type)) {
        fail("'" + target.name + "' holds " + typeName(type) + ", but this is " + typeName(valueType), *statement.value);
    }

    emit(storeOpcode, slot);
}

void Emitter::emitAssignField(const Statement& statement)
{
    const Expression& target = *statement.target;
    const FieldEntry& field  = nodeField(target);

    const int nodeSlot = allocateSlot();

    if (field.access == FieldAccess::ReadOnly) {
        fail("'" + target.name + "' can only be read; a played note's values are passed to playNote", target);
    }

    if (emitExpression(*target.left) != ValueType::Node) {
        fail("only a Node has properties", *target.left);
    }

    emit(ScriptOpcode::StoreLocal, nodeSlot);
    emit(ScriptOpcode::PushLocal, nodeSlot);

    if (statement.op != TokenKind::Assign) {
        emit(ScriptOpcode::PushLocal, nodeSlot);
        emit(ScriptOpcode::PushNodeField, static_cast<int>(field.field));
    }

    ValueType valueType = emitExpression(*statement.value);

    if (statement.op != TokenKind::Assign) {
        valueType = commonType(field.type, valueType);
    }

    if (statement.op == TokenKind::PlusAssign) {
        emit(ScriptOpcode::Add, static_cast<int>(numberKind(valueType)));
    }

    if (statement.op == TokenKind::MinusAssign) {
        emit(ScriptOpcode::Subtract, static_cast<int>(numberKind(valueType)));
    }

    if (!emitConversion(valueType, field.type)) {
        fail("'" + target.name + "' holds " + typeName(field.type), *statement.value);
    }

    emit(ScriptOpcode::StoreNodeField, static_cast<int>(field.field));

    --nextSlot;
}

void Emitter::emitIf(const Statement& statement)
{
    const ValueType conditionType = emitExpression(*statement.value);

    if (commonType(conditionType, conditionType) == ValueType::Void) {
        fail("a condition must be a number; compare a Node with '==' or '!='", *statement.value);
    }

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

void Emitter::emitBlock(std::span<const StatementPtr> body)
{
    openScope();

    emitSequence(body);

    closeScope();
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

void Emitter::patch(int jumpIndex, int target)
{
    script.instructions[static_cast<std::size_t>(jumpIndex)].operand = target;
}

void Emitter::emitFor(const Statement& statement)
{
    openScope();

    const int parentSlot = allocateSlot();
    const int indexSlot  = allocateSlot();

    if (statement.target == nullptr) {
        emit(ScriptOpcode::PushContext, static_cast<int>(ScriptContextValue::Current));
    }
    else if (emitExpression(*statement.target) != ValueType::Node) {
        fail("only a Node has children", *statement.target);
    }

    emit(ScriptOpcode::StoreLocal, parentSlot);
    emit(ScriptOpcode::PushInt, 0);
    emit(ScriptOpcode::StoreLocal, indexSlot);

    const int loopTop = here();

    emit(ScriptOpcode::PushLocal, indexSlot);
    emit(ScriptOpcode::PushLocal, parentSlot);
    emit(ScriptOpcode::PushNodeField, static_cast<int>(ScriptField::ChildCount));
    emit(ScriptOpcode::Less);

    const int exitJump = emit(ScriptOpcode::JumpIfFalse);

    emit(ScriptOpcode::PushLocal, parentSlot);
    emit(ScriptOpcode::PushLocal, indexSlot);
    emit(ScriptOpcode::ChildAt);
    emit(ScriptOpcode::StoreLocal, declareLocal(statement.name, ValueType::Node));

    loopStack.push_back({});

    emitBlock(statement.body);

    LoopFrame frame = std::move(loopStack.back());
    loopStack.pop_back();

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

void Emitter::patchLoopFrame(const LoopFrame& frame, int continueTarget, int exitTarget)
{
    for (const int jumpIndex : frame.continueJumps) {
        patch(jumpIndex, continueTarget);
    }

    for (const int jumpIndex : frame.breakJumps) {
        patch(jumpIndex, exitTarget);
    }
}

void Emitter::emitWhile(const Statement& statement)
{
    const int loopTop = here();

    const ValueType conditionType = emitExpression(*statement.value);

    if (commonType(conditionType, conditionType) == ValueType::Void) {
        fail("a condition must be a number; compare a Node with '==' or '!='", *statement.value);
    }

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

void Emitter::emitBreak(const Statement& statement)
{
    if (loopStack.empty()) {
        fail("'break' is only valid inside a loop", statement);
    }

    loopStack.back().breakJumps.push_back(emit(ScriptOpcode::Jump));
}

void Emitter::fail(const std::string& message, const Statement& statement)
{
    diagnostics.push_back({ message, statement.line, statement.column, statement.length });
    throw EmitFailure{};
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
    const ValueType returnType = currentFunction->returnType;

    if (statement.value == nullptr) {
        if (returnType != ValueType::Void) {
            fail("'" + currentFunction->name + "' must return " + typeName(returnType), statement);
        }

        emit(ScriptOpcode::PushInt, 0);
        emit(ScriptOpcode::Return);
        return;
    }

    if (returnType == ValueType::Void) {
        fail("'" + currentFunction->name + "' gives no value; use 'return;'", *statement.value);
    }

    if (!emitConversion(emitExpression(*statement.value), returnType)) {
        fail("'" + currentFunction->name + "' must return " + typeName(returnType), *statement.value);
    }

    emit(ScriptOpcode::Return);
}
