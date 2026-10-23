#pragma once

#include <vector>

/*
    The compiled form of a traversal script, which is the only thing of the scripting language that
    reaches the audio thread. An RTScript is a program for a small stack machine: instructions
    (a ScriptOpcode and one operand) in one list, a ScriptFunction giving each function's entry
    point, parameter count and locals, and the starting values of the script's member variables
    (memberDefaults), global variables (globalDefaults) and numeric constants. mainFunction and
    advanceFunction are the functions the script defines for playing and for choosing the next
    node; memberInitFunction and globalInitFunction run the variables' starting expressions.

    ScriptField and ScriptContextValue name the node properties and traversal values a script can
    read, as numbers the instructions carry. The max constants cap locals, stack depth, members and
    call depth, and stepBudget caps how many instructions one call may run, so a script cannot
    hang playback.

    Emitter writes it, AudioSnapshotPublisher publishes it, and ScriptRun executes it. It holds only
    numbers and vectors, so it is one of the two kinds of type that cross the audio boundary, with
    the RTData structs.
*/

enum class ScriptOpcode
{
    Halt,

    PushInt,
    PushReal,
    PushLocal,
    PushMember,
    PushGlobal,
    PushContext,
    PushNodeField,
    StoreLocal,
    StoreMember,
    StoreGlobal,
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
    Double,
    Bool
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
    Eligible,
    TraversalId
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
    std::vector<double>            globalDefaults;
    std::vector<double>            constants;

    int mainFunction       = -1;
    int advanceFunction    = -1;
    int memberInitFunction = -1;
    int globalInitFunction = -1;

    int stepBudget = defaultStepBudget;

    bool isEmpty() const { return instructions.empty(); }
};
