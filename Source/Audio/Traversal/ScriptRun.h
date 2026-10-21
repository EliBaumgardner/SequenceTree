#pragma once

#include <array>
#include <span>

#include "../../Script/RTScript.h"
#include "TraversalRule.h"

/*
    The interpreter for compiled traversal scripts: it executes a script's instructions against one
    traversal and returns what the script decided.

    A ScriptRun is made for one ScriptRunContext: the RTScript, the graph, the traversal's
    NodeStateTable and variables, and whether its writes are kept. call runs one of the script's
    functions with the given arguments and returns its result, which is how a script chooses where
    the walk goes and what it plays. A call stops after the script's stepBudget instructions, so an
    endless loop cannot hang playback, and it returns -1 if the script runs that long or hits an
    error. It uses only fixed-size memory, so it is safe on the audio thread.
    TraversalLogic::makeScriptContext gathers what the script can see and change into a
    ScriptRunContext.

    With ScriptWrites::Commit, the changes a script makes to counts and variables are kept. With
    ScriptWrites::Trial they are thrown away when the run ends, so TraversalLogic::peekNextTarget
    can ask where the walk would go without moving it.

    ScriptHost is how a script tells playback to advance, asks a note's length, or plays a note,
    and TraversalDispatcher implements it. ScriptGlobals are the script's variables shared by every
    traversal in a pool, and ScriptMembers are the variables of one traversal.
*/

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

    virtual int  advance     (int steps) = 0;
    virtual int  noteDuration(int nodeId) = 0;
    virtual void playNote    (const ScriptNote& note) = 0;
};

using ScriptMembers = std::array<double, RTScript::maxMembers>;

struct ScriptGlobals
{
    ScriptMembers   values {};
    const RTScript* script = nullptr;
};

struct ScriptRunContext
{
    const RTScript& script;
    const NodeMap&  nodes;

    NodeStateTable& nodeState;
    ScriptMembers&  members;

    TraversalKey traversalKey;
    int          randomValue = 0;

    int            currentNodeId = -1;
    NodeStateSlot  countSlot     = NodeStateSlot::Count;
    ChildPredicate isEligible    = nullptr;

    ScriptWrites writes = ScriptWrites::Commit;
    ScriptHost*  host   = nullptr;

    ScriptGlobals* globals = nullptr;
};

class ScriptRun
{
public:

    explicit ScriptRun(const ScriptRunContext& runContext);

    int call(int functionIndex, std::span<const int> arguments);

private:

    enum class Progress;

    bool enterFunction(int functionIndex, int returnAddress, int& programCounter);

    Progress execute          (const ScriptInstruction& instruction, int& programCounter, int& result);
    Progress popValue         (const ScriptInstruction& instruction, int& programCounter, int& result);
    Progress pushValue        (const ScriptInstruction& instruction);
    Progress executeNodeOpcode(const ScriptInstruction& instruction);
    Progress drawRandom       ();

    int readField  (ScriptField field, int nodeId);
    int noteValue  (ScriptField field, const RTNode& node) const;
    int readContext(ScriptContextValue value) const;
    int readState  (NodeStateSlot slot, int nodeId) const;
    int childAt    (int parentId, int childIndex) const;

    void writeField(ScriptField field,  int nodeId, int value);
    void writeState(NodeStateSlot slot, int nodeId, int value);

    static double convert       (ScriptNumber number, double value);
    static double calculate     (const ScriptInstruction& instruction, double left, double right);
    static double realArithmetic(ScriptOpcode opcode, double left, double right);
    static int    arithmetic    (ScriptOpcode opcode, int left, int right);

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

    static constexpr int stackCapacity      = RTScript::maxStack * RTScript::maxCallDepth;
    static constexpr int localCapacity      = RTScript::maxLocals * RTScript::maxCallDepth;
    static constexpr int trialWriteCapacity = 64;

    ScriptRunContext context;

    ScriptMembers  trialMembers {};
    ScriptMembers  trialGlobals {};
    ScriptMembers* memberValues = nullptr;
    ScriptMembers* globalValues = nullptr;

    std::array<double, stackCapacity>          stack {};
    std::array<double, localCapacity>          locals {};
    std::array<Frame, RTScript::maxCallDepth>  frames {};
    std::array<TrialWrite, trialWriteCapacity> trialWrites {};

    int stackTop        = 0;
    int frameCount      = 0;
    int trialWriteCount = 0;

    unsigned int randomState = 1;
};
