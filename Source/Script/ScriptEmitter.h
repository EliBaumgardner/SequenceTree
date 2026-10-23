#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include "ScriptSyntaxTree.h"

/*
    The last phase of the compiler: it walks the ClassDeclaration and writes the RTScript, the only
    thing that leaves Source/Script. run declares the globals, members and functions, emits each
    function's instructions, and then emits the two functions that set the variables' starting
    values (globalInitFunction and memberInitFunction). While emitting it checks what the Parser
    could not: every name is declared, types agree (it inserts a conversion between int, float and
    double where one is allowed), Node properties exist and are only assigned where they are
    writable (nodeFieldTable), and calls are valid: advance, playNote, user functions with the
    right arguments, and core::random after 'import core'.

    Each error becomes a ScriptDiagnostic that marks the text it concerns. The Emitter also
    counts the deepest stack and most locals a function uses, and rejects a script that would
    exceed RTScript's limits. An Emitter lives for one compile, made by compileTraversalScript.
*/

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

    void declareMembers  (std::span<const MemberDeclaration> declarations, std::vector<LocalBinding>& bindings, std::vector<double>& defaults);
    void declareFunctions(const ClassDeclaration& declaration);

    void emitFunction   (const FunctionDeclaration& declaration, const FunctionSignature& signature);

    void emitSequence   (std::span<const StatementPtr> statements);
    void emitBlock      (std::span<const StatementPtr> body);

    void emitStatement  (const Statement& statement);
    void emitDeclare    (const Statement& statement);
    void emitAssign     (const Statement& statement);
    void emitAssignField(const Statement& statement);

    void emitIf         (const Statement& statement);
    void emitFor        (const Statement& statement);
    void emitWhile      (const Statement& statement);

    void emitBreak      (const Statement& statement);
    void emitContinue   (const Statement& statement);
    void emitReturn     (const Statement& statement);

    void openScope ();
    void closeScope();

    void patch          (int jumpIndex, int target);
    void patchLoopFrame (const LoopFrame& frame, int continueTarget, int exitTarget);

    [[noreturn]] void fail(const std::string& message, const Expression& expression);
    [[noreturn]] void fail(const std::string& message, const Statement& statement);

    const LocalBinding*      findMember  (const std::string& name) const;
    const LocalBinding*      findGlobal  (const std::string& name) const;
    const LocalBinding*      findLocal   (const std::string& name) const;
    const FunctionSignature* findFunction(const std::string& name) const;

    int emitInitializer(std::span<const MemberDeclaration> declarations, std::span<const LocalBinding> bindings, ScriptOpcode storeOpcode);

    int emit(ScriptOpcode opcode, int operand = 0);
    int here() const;

    int declareLocal(const std::string& name, ValueType type);
    int allocateSlot();

    ValueType emitExpression(const Expression& expression);
    ValueType emitName      (const Expression& expression);
    ValueType emitField     (const Expression& expression);
    ValueType emitCall      (const Expression& expression);

    ValueType emitUnary (const Expression& expression);
    ValueType emitBinary(const Expression& expression);

    const FieldEntry& nodeField(const Expression& expression);

    ScriptOpcode binaryOpcode(const Expression& expression);

    bool emitConversion(ValueType from, ValueType to);

    static int          stackDelta(ScriptOpcode opcode);
    static ValueType    commonType(ValueType left, ValueType right);
    static ScriptNumber numberKind(ValueType type);
    static const char*  typeName  (ValueType type);

    RTScript&                      script;
    std::vector<ScriptDiagnostic>& diagnostics;

    std::vector<std::string>       imports;
    std::vector<LocalBinding>      locals;
    std::vector<LocalBinding>      members;
    std::vector<LocalBinding>      globals;
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
