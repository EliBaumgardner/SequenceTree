#pragma once

#include <array>
#include <span>

#include "../Script/RTScript.h"
#include "TraversalRule.h"

enum class ScriptWrites { Commit, Trial };

struct ScriptNote
{
    enum class Kind { Default, Played, Silent };

    Kind kind     = Kind::Default;
    int  nodeId   = -1;
    int  pitch    = 0;
    int  duration = 0;
    int  velocity = 0;
};

class ScriptHost
{
public:

    virtual ~ScriptHost() = default;

    virtual int  advance(int steps) = 0;
    virtual void playNote(const ScriptNote& note) = 0;
    virtual int  noteDuration(int nodeId) = 0;
};

using ScriptMembers = std::array<double, RTScript::maxMembers>;

struct ScriptRunContext
{
    const RTScript& script;
    const NodeMap&  nodes;
    NodeStateTable& nodeState;
    ScriptMembers&  members;
    TraversalKey    traversalKey;
    int             randomValue   = 0;
    int             currentNodeId = -1;
    NodeStateSlot   countSlot     = NodeStateSlot::Count;
    ChildPredicate  isEligible    = nullptr;
    ScriptWrites    writes        = ScriptWrites::Commit;
    ScriptHost*     host          = nullptr;
};

class ScriptRun
{
public:

    explicit ScriptRun(const ScriptRunContext& runContext);

    int call(int functionIndex, std::span<const int> arguments);

private:

    enum class Progress { Running, Returned, Faulted };

    struct Frame
    {
        int returnAddress = -1;
        int localBase     = 0;
        int stackBase     = 0;
    };

    struct TrialWrite
    {
        NodeStateSlot slot   = NodeStateSlot::Count;
        int           nodeId = -1;
        int           value  = 0;
    };

    Progress execute(const ScriptInstruction& instruction, int& programCounter, int& result);
    Progress pushValue(const ScriptInstruction& instruction);
    Progress popValue(const ScriptInstruction& instruction, int& programCounter, int& result);
    Progress executeNodeOpcode(const ScriptInstruction& instruction);

    bool enterFunction(int functionIndex, int returnAddress, int& programCounter);

    static double calculate(const ScriptInstruction& instruction, double left, double right);
    static int    arithmetic(ScriptOpcode opcode, int left, int right);
    static double realArithmetic(ScriptOpcode opcode, double left, double right);
    static double convert(ScriptNumber number, double value);

    int  readContext(ScriptContextValue value) const;
    int  readField  (ScriptField field, int nodeId);
    void writeField (ScriptField field, int nodeId, int value);
    int  readState  (NodeStateSlot slot, int nodeId) const;
    void writeState (NodeStateSlot slot, int nodeId, int value);
    int  noteValue  (ScriptField field, const RTNode& node) const;
    int  childAt    (int parentId, int childIndex) const;

    static constexpr int stackCapacity      = RTScript::maxStack * RTScript::maxCallDepth;
    static constexpr int localCapacity      = RTScript::maxLocals * RTScript::maxCallDepth;
    static constexpr int trialWriteCapacity = 64;

    ScriptRunContext context;

    ScriptMembers  trialMembers {};
    ScriptMembers* memberValues = nullptr;

    std::array<double, stackCapacity>                  stack {};
    std::array<double, localCapacity>                  locals {};
    std::array<Frame, RTScript::maxCallDepth>          frames {};
    std::array<TrialWrite, trialWriteCapacity>         trialWrites {};

    int stackTop        = 0;
    int frameCount      = 0;
    int trialWriteCount = 0;
};
