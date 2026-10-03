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

    void declareMembers  (const ClassDeclaration& declaration);
    const LocalBinding*      findMember  (const std::string& name) const;
    void declareFunctions(const ClassDeclaration& declaration);
    const FunctionSignature* findFunction(const std::string& name) const;
    void emitFunction    (const FunctionDeclaration& declaration, const FunctionSignature& signature);
    int here() const;
    const LocalBinding*      findLocal   (const std::string& name) const;
    int declareLocal(const std::string& name, ValueType type);
    int allocateSlot();
    void emitSequence(std::span<const StatementPtr> statements);
    void emitStatement(const Statement& statement);
    void emitDeclare(const Statement& statement);
    int emit(ScriptOpcode opcode, int operand = 0);
    static int stackDelta(ScriptOpcode opcode);
    ValueType emitExpression(const Expression& expression);
    ValueType emitName(const Expression& expression);
    [[noreturn]] void fail(const std::string& message, const Expression& expression);
    ValueType emitField(const Expression& expression);
    const FieldEntry& nodeField(const Expression& expression);
    ValueType emitCall(const Expression& expression);
    bool emitConversion(ValueType from, ValueType to);
    static ValueType commonType(ValueType left, ValueType right);
    static ScriptNumber numberKind(ValueType type);
    static const char* typeName(ValueType type);
    ValueType emitUnary(const Expression& expression);
    ValueType emitBinary(const Expression& expression);
    ScriptOpcode binaryOpcode(const Expression& expression);
    void emitAssign(const Statement& statement);
    void emitAssignField(const Statement& statement);
    void emitIf(const Statement& statement);
    void emitBlock(std::span<const StatementPtr> body);
    void openScope();
    void closeScope();
    void patch(int jumpIndex, int target);
    void emitFor(const Statement& statement);
    void patchLoopFrame(const LoopFrame& frame, int continueTarget, int exitTarget);
    void emitWhile(const Statement& statement);
    void emitBreak(const Statement& statement);
    [[noreturn]] void fail(const std::string& message, const Statement& statement);
    void emitContinue(const Statement& statement);
    void emitReturn(const Statement& statement);

    RTScript&                      script;
    std::vector<ScriptDiagnostic>& diagnostics;

    std::vector<std::string>       imports;
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
