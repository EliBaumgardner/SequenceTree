#pragma once

#include <vector>

enum class ScriptOpcode
{
    Halt,

    PushInt,
    PushReal,
    PushLocal,
    PushMember,
    PushContext,
    PushNodeField,
    StoreLocal,
    StoreMember,
    StoreNodeField,
    Pop,

    ChildAt,

    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Negate,
    Convert,

    Equal,
    NotEqual,
    Less,
    LessOrEqual,
    Greater,
    GreaterOrEqual,

    LogicalAnd,
    LogicalOr,
    LogicalNot,

    Jump,
    JumpIfFalse,
    JumpIfTrue,

    Call,
    Advance,
    PlayNote,

    CoreRandom,

    Return
};

enum class ScriptNumber
{
    Int,
    Float,
    Double
};

enum class ScriptContextValue
{
    Current,
    TraversalId,
    TraversalRandom,
    TraversalInstance
};

enum class ScriptField
{
    Id,
    Pitch,
    Velocity,
    Duration,

    Count,
    SwitchCount,
    TriggerCount,
    SubLoopCount,

    CountLimit,
    TriggerLimit,
    SwitchLimit,
    SubLoopLimit,
    Repeat,
    Probability,
    ChildCount,
    LastChild,
    Parent,
    Eligible
};

struct ScriptInstruction
{
    ScriptOpcode opcode  = ScriptOpcode::Halt;
    int          operand = 0;

    ScriptInstruction() = default;

    ScriptInstruction(ScriptOpcode instructionOpcode)
        : opcode(instructionOpcode) {}

    ScriptInstruction(ScriptOpcode instructionOpcode, int instructionOperand)
        : opcode(instructionOpcode), operand(instructionOperand) {}
};

struct ScriptFunction
{
    int entry          = 0;
    int parameterCount = 0;
    int localCount     = 0;
};

struct RTScript
{
    static constexpr int maxLocals         = 32;
    static constexpr int maxStack          = 64;
    static constexpr int maxMembers        = 32;
    static constexpr int maxCallDepth      = 8;
    static constexpr int defaultStepBudget = 8192;

    std::vector<ScriptInstruction> instructions;
    std::vector<ScriptFunction>    functions;
    std::vector<double>            memberDefaults;
    std::vector<double>            constants;

    int mainFunction    = -1;
    int advanceFunction = -1;

    int stepBudget = defaultStepBudget;

    bool isEmpty() const { return instructions.empty(); }
};
