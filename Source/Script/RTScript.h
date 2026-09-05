#pragma once

#include <vector>

enum class ScriptOpcode
{
    Halt,

    PushInt,
    PushLocal,
    PushField,
    StoreLocal,
    Pop,

    LoadChild,

    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Negate,

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

    Return
};

enum class ScriptField
{
    ParentId,
    ParentCount,
    ParentChildCount,
    ParentLastChosenChild,

    TraversalId,

    ChildId,
    ChildIsEligible,
    ChildCountLimit,
    ChildTriggerLimit,
    ChildTriggerCount,
    ChildVisitCount,
    ChildRepeatValue,
    ChildPitchOffset,
    ChildSwitchCountLimit,
    ChildSubLoopCountLimit
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

    ScriptInstruction(ScriptOpcode instructionOpcode, ScriptField field)
        : opcode(instructionOpcode), operand(static_cast<int>(field)) {}
};

struct RTScript
{
    static constexpr int maxLocals  = 32;
    static constexpr int maxStack   = 64;
    static constexpr int defaultStepBudget = 8192;

    std::vector<ScriptInstruction> instructions;

    int localCount = 0;
    int stepBudget = defaultStepBudget;

    bool isEmpty() const { return instructions.empty(); }
};

RTScript makeNativeSelectChildScript();
