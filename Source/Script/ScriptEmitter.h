#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include "ScriptParser.h"

struct LoopFrame
{
    std::vector<int> breakJumps;
    std::vector<int> continueJumps;
};

struct LocalBinding
{
    std::string name;
    int         slot = 0;
    ValueType   type = ValueType::Int;
};

struct FunctionSignature
{
    std::string            name;
    ValueType              returnType = ValueType::Void;
    std::vector<ValueType> parameterTypes;
    int                    index       = 0;
    int                    declaration = 0;
};

enum class FieldAccess { ReadOnly, Writable };

struct FieldEntry
{
    const char* name;
    ScriptField field;
    ValueType   type;
    FieldAccess access;
};

struct ContextEntry
{
    const char*        name;
    ScriptContextValue value;
};

struct EmitFailure {};

class Emitter
{
public:

    Emitter(RTScript& target, std::vector<ScriptDiagnostic>& diagnosticList)
        : script(target), diagnostics(diagnosticList) {}

    void run(const ClassDeclaration& declaration);

private:

    static int stackDelta(ScriptOpcode opcode);

    static const char* typeName(ValueType type);

    static ScriptNumber numberKind(ValueType type);

    static ValueType commonType(ValueType left, ValueType right);

    int emit(ScriptOpcode opcode, int operand = 0);

    bool emitConversion(ValueType from, ValueType to);

    int here() const;

    void patch(int jumpIndex, int target);

    [[noreturn]] void fail(const std::string& message, const Statement& statement);
    [[noreturn]] void fail(const std::string& message, const Expression& expression);

    int allocateSlot();
    int declareLocal(const std::string& name, ValueType type);

    const LocalBinding*      findLocal   (const std::string& name) const;
    const LocalBinding*      findMember  (const std::string& name) const;
    const FunctionSignature* findFunction(const std::string& name) const;

    void openScope();
    void closeScope();

    void declareMembers  (const ClassDeclaration& declaration);
    void declareFunctions(const ClassDeclaration& declaration);
    void emitFunction    (const FunctionDeclaration& declaration, const FunctionSignature& signature);

    void emitSequence(std::span<const StatementPtr> statements);
    void emitBlock(std::span<const StatementPtr> body);

    void emitStatement(const Statement& statement);
    void emitDeclare(const Statement& statement);
    void emitAssign(const Statement& statement);
    void emitAssignField(const Statement& statement);
    void emitIf(const Statement& statement);
    void emitFor(const Statement& statement);
    void emitWhile(const Statement& statement);
    void emitBreak(const Statement& statement);
    void emitContinue(const Statement& statement);
    void emitReturn(const Statement& statement);

    void patchLoopFrame(const LoopFrame& frame, int continueTarget, int exitTarget);

    ValueType emitExpression(const Expression& expression);
    ValueType emitName(const Expression& expression);
    ValueType emitField(const Expression& expression);
    ValueType emitCall(const Expression& expression);
    ValueType emitUnary(const Expression& expression);
    ValueType emitBinary(const Expression& expression);

    const FieldEntry& nodeField(const Expression& expression);

    ScriptOpcode binaryOpcode(const Expression& expression);

    RTScript&                      script;
    std::vector<ScriptDiagnostic>& diagnostics;

    std::vector<LocalBinding>      locals;
    std::vector<LocalBinding>      members;
    std::vector<FunctionSignature> functions;

    std::vector<std::size_t> scopeMarks;
    std::vector<int>         slotMarks;

    std::vector<LoopFrame> loopStack;

    const FunctionSignature* currentFunction = nullptr;

    int nextSlot      = 0;
    int highWaterSlot = 0;

    int stackDepth     = 0;
    int highWaterStack = 0;
};
