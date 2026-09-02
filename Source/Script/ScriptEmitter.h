#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "ScriptParser.h"

namespace script {

struct LoopFrame
{
    std::vector<int> breakJumps;
    std::vector<int> continueJumps;
};

struct LocalBinding
{
    std::string name;
    int         slot = 0;
};

class Emitter
{
public:

    Emitter(RTScript& target, std::vector<ScriptDiagnostic>& diagnosticList)
        : script(target), diagnostics(diagnosticList) {}

    void run(const std::vector<StatementPtr>& program);

private:

    static int stackDelta(ScriptOpcode opcode);

    int emit(ScriptOpcode opcode, int operand = 0);
    int emit(ScriptOpcode opcode, ScriptField field);

    int here() const;

    void patch(int jumpIndex, int target);

    [[noreturn]] void fail(const std::string& message, const Statement& statement);
    [[noreturn]] void fail(const std::string& message, const Expression& expression);

    int allocateSlot();
    int declareLocal(const std::string& name);

    bool findLocal(const std::string& name, int& slot) const;

    void openScope();
    void closeScope();

    void emitSequence(const std::vector<StatementPtr>& statements);
    void emitBlock(const std::vector<StatementPtr>& body);

    void emitStatement(const Statement& statement);
    void emitLet(const Statement& statement);
    void emitAssign(const Statement& statement);
    void emitIf(const Statement& statement);
    void emitFor(const Statement& statement);
    void emitWhile(const Statement& statement);
    void emitBreak(const Statement& statement);
    void emitContinue(const Statement& statement);
    void emitReturn(const Statement& statement);

    void patchLoopFrame(const LoopFrame& frame, int continueTarget, int exitTarget);

    void emitExpression(const Expression& expression);
    void emitLiteral(const Expression& expression);
    void emitName(const Expression& expression);
    void emitMember(const Expression& expression);
    void emitUnary(const Expression& expression);
    void emitBinary(const Expression& expression);

    static ScriptOpcode binaryOpcode(TokenKind kind);

    RTScript&                      script;
    std::vector<ScriptDiagnostic>& diagnostics;

    std::vector<LocalBinding> locals;
    std::vector<std::size_t>  scopeMarks;
    std::vector<int>          slotMarks;

    std::vector<LoopFrame> loopStack;

    std::string childBinding;

    int nextSlot      = 0;
    int highWaterSlot = 0;

    int stackDepth     = 0;
    int highWaterStack = 0;
};

}
