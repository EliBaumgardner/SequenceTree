#include "ScriptRun.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

ScriptRun::ScriptRun(const ScriptRunContext& runContext)
    : context(runContext)
{
    memberValues = &context.members;

    if (context.writes == ScriptWrites::Trial) {
        trialMembers = context.members;
        memberValues = &trialMembers;
    }
}

int ScriptRun::call(int functionIndex, std::span<const int> arguments)
{
    const int instructionCount = static_cast<int>(context.script.instructions.size());

    int programCounter = 0;
    int stepsRemaining = context.script.stepBudget;
    int result         = -1;

    if (static_cast<int>(arguments.size()) > RTScript::maxStack) {
        return -1;
    }

    for (const int argument : arguments) {
        stack[static_cast<std::size_t>(stackTop++)] = argument;
    }

    if (!enterFunction(functionIndex, -1, programCounter)) {
        return -1;
    }

    while (programCounter >= 0 && programCounter < instructionCount) {
        if (--stepsRemaining < 0) {
            return -1;
        }

        const ScriptInstruction instruction = context.script.instructions[static_cast<std::size_t>(programCounter)];

        ++programCounter;

        const Progress progress = execute(instruction, programCounter, result);

        if (progress == Progress::Returned) {
            return result;
        }

        if (progress == Progress::Faulted) {
            return -1;
        }
    }

    return -1;
}

ScriptRun::Progress ScriptRun::execute(const ScriptInstruction& instruction, int& programCounter, int& result)
{
    switch (instruction.opcode) {
        case ScriptOpcode::PushInt:
        case ScriptOpcode::PushReal:
        case ScriptOpcode::PushLocal:
        case ScriptOpcode::PushMember:
        case ScriptOpcode::PushContext:
            return pushValue(instruction);

        case ScriptOpcode::StoreLocal:
        case ScriptOpcode::StoreMember:
        case ScriptOpcode::Pop:
        case ScriptOpcode::JumpIfFalse:
        case ScriptOpcode::JumpIfTrue:
        case ScriptOpcode::Return:
            return popValue(instruction, programCounter, result);

        case ScriptOpcode::PushNodeField:
        case ScriptOpcode::StoreNodeField:
        case ScriptOpcode::ChildAt:
        case ScriptOpcode::Advance:
        case ScriptOpcode::PlayNote:
            return executeNodeOpcode(instruction);

        case ScriptOpcode::Jump: {
            programCounter = instruction.operand;
            return Progress::Running;
        }

        case ScriptOpcode::Call: {
            if (!enterFunction(instruction.operand, programCounter, programCounter)) {
                return Progress::Faulted;
            }

            return Progress::Running;
        }

        case ScriptOpcode::Negate:
        case ScriptOpcode::LogicalNot: {
            if (stackTop < 1) {
                return Progress::Faulted;
            }

            double& value = stack[static_cast<std::size_t>(stackTop - 1)];

            value = calculate(instruction, 0.0, value);
            return Progress::Running;
        }

        case ScriptOpcode::Convert: {
            if (stackTop < 1) {
                return Progress::Faulted;
            }

            double& value = stack[static_cast<std::size_t>(stackTop - 1)];

            value = convert(static_cast<ScriptNumber>(instruction.operand), value);
            return Progress::Running;
        }

        case ScriptOpcode::Halt:
            return Progress::Faulted;

        default:
            break;
    }

    if (stackTop < 2) {
        return Progress::Faulted;
    }

    const double right = stack[static_cast<std::size_t>(--stackTop)];
    double&      left  = stack[static_cast<std::size_t>(stackTop - 1)];

    left = calculate(instruction, left, right);
    return Progress::Running;
}

ScriptRun::Progress ScriptRun::pushValue(const ScriptInstruction& instruction)
{
    const int operand   = instruction.operand;
    const int localBase = frames[static_cast<std::size_t>(frameCount - 1)].localBase;

    double value = operand;

    if (stackTop >= stackCapacity) {
        return Progress::Faulted;
    }

    if (instruction.opcode == ScriptOpcode::PushReal) {
        if (operand < 0 || operand >= static_cast<int>(context.script.constants.size())) {
            return Progress::Faulted;
        }

        value = context.script.constants[static_cast<std::size_t>(operand)];
    }

    if (instruction.opcode == ScriptOpcode::PushLocal) {
        if (operand < 0 || operand >= RTScript::maxLocals) {
            return Progress::Faulted;
        }

        value = locals[static_cast<std::size_t>(localBase + operand)];
    }

    if (instruction.opcode == ScriptOpcode::PushMember) {
        if (operand < 0 || operand >= RTScript::maxMembers) {
            return Progress::Faulted;
        }

        value = (*memberValues)[static_cast<std::size_t>(operand)];
    }

    if (instruction.opcode == ScriptOpcode::PushContext) {
        value = readContext(static_cast<ScriptContextValue>(operand));
    }

    stack[static_cast<std::size_t>(stackTop++)] = value;
    return Progress::Running;
}

ScriptRun::Progress ScriptRun::popValue(const ScriptInstruction& instruction, int& programCounter, int& result)
{
    const int operand   = instruction.operand;
    const int localBase = frames[static_cast<std::size_t>(frameCount - 1)].localBase;

    if (stackTop < 1) {
        return Progress::Faulted;
    }

    const double value = stack[static_cast<std::size_t>(--stackTop)];

    if (instruction.opcode == ScriptOpcode::StoreLocal) {
        if (operand < 0 || operand >= RTScript::maxLocals) {
            return Progress::Faulted;
        }

        locals[static_cast<std::size_t>(localBase + operand)] = value;
    }

    if (instruction.opcode == ScriptOpcode::StoreMember) {
        if (operand < 0 || operand >= RTScript::maxMembers) {
            return Progress::Faulted;
        }

        (*memberValues)[static_cast<std::size_t>(operand)] = value;
    }

    if ((instruction.opcode == ScriptOpcode::JumpIfFalse && value == 0)
        || (instruction.opcode == ScriptOpcode::JumpIfTrue && value != 0)) {
        programCounter = operand;
    }

    if (instruction.opcode == ScriptOpcode::Return) {
        const Frame frame = frames[static_cast<std::size_t>(--frameCount)];

        stackTop = frame.stackBase;

        if (frameCount == 0) {
            result = static_cast<int>(convert(ScriptNumber::Int, value));
            return Progress::Returned;
        }

        programCounter = frame.returnAddress;
        stack[static_cast<std::size_t>(stackTop++)] = value;
    }

    return Progress::Running;
}

ScriptRun::Progress ScriptRun::executeNodeOpcode(const ScriptInstruction& instruction)
{
    const ScriptField field = static_cast<ScriptField>(instruction.operand);

    switch (instruction.opcode) {
        case ScriptOpcode::PushNodeField: {
            if (stackTop < 1) {
                return Progress::Faulted;
            }

            double& top = stack[static_cast<std::size_t>(stackTop - 1)];

            top = readField(field, static_cast<int>(top));
            return Progress::Running;
        }

        case ScriptOpcode::StoreNodeField: {
            if (stackTop < 2) {
                return Progress::Faulted;
            }

            const int value  = static_cast<int>(stack[static_cast<std::size_t>(--stackTop)]);
            const int nodeId = static_cast<int>(stack[static_cast<std::size_t>(--stackTop)]);

            writeField(field, nodeId, value);
            return Progress::Running;
        }

        case ScriptOpcode::ChildAt: {
            if (stackTop < 2) {
                return Progress::Faulted;
            }

            const int childIndex = static_cast<int>(stack[static_cast<std::size_t>(--stackTop)]);
            double&   top        = stack[static_cast<std::size_t>(stackTop - 1)];

            top = childAt(static_cast<int>(top), childIndex);
            return Progress::Running;
        }

        case ScriptOpcode::Advance: {
            if (stackTop < 1) {
                return Progress::Faulted;
            }

            double& top = stack[static_cast<std::size_t>(stackTop - 1)];

            if (context.host == nullptr) {
                top = -1;
                return Progress::Running;
            }

            context.currentNodeId = context.host->advance(static_cast<int>(top));
            top                   = context.currentNodeId;
            return Progress::Running;
        }

        case ScriptOpcode::PlayNote: {
            if (stackTop < 4) {
                return Progress::Faulted;
            }

            stackTop -= 4;

            if (context.host != nullptr) {
                context.host->playNote({ ScriptNote::Kind::Played,
                                         static_cast<int>(stack[static_cast<std::size_t>(stackTop)]),
                                         static_cast<int>(stack[static_cast<std::size_t>(stackTop + 1)]),
                                         static_cast<int>(stack[static_cast<std::size_t>(stackTop + 2)]),
                                         static_cast<int>(stack[static_cast<std::size_t>(stackTop + 3)]) });
            }

            stack[static_cast<std::size_t>(stackTop++)] = 0;
            return Progress::Running;
        }

        default:
            return Progress::Faulted;
    }
}

bool ScriptRun::enterFunction(int functionIndex, int returnAddress, int& programCounter)
{
    const int functionCount = static_cast<int>(context.script.functions.size());
    const int localBase     = frameCount * RTScript::maxLocals;

    if (functionIndex < 0 || functionIndex >= functionCount || frameCount >= RTScript::maxCallDepth) {
        return false;
    }

    const ScriptFunction& function = context.script.functions[static_cast<std::size_t>(functionIndex)];

    if (stackTop < function.parameterCount || function.parameterCount > RTScript::maxLocals) {
        return false;
    }

    for (int slot = 0; slot < RTScript::maxLocals; ++slot) {
        locals[static_cast<std::size_t>(localBase + slot)] = 0;
    }

    stackTop -= function.parameterCount;

    for (int parameter = 0; parameter < function.parameterCount; ++parameter) {
        locals[static_cast<std::size_t>(localBase + parameter)] = stack[static_cast<std::size_t>(stackTop + parameter)];
    }

    frames[static_cast<std::size_t>(frameCount++)] = { returnAddress, localBase, stackTop };

    programCounter = function.entry;
    return true;
}

double ScriptRun::calculate(const ScriptInstruction& instruction, double left, double right)
{
    const ScriptNumber number = static_cast<ScriptNumber>(instruction.operand);

    if (number == ScriptNumber::Int) {
        return arithmetic(instruction.opcode, static_cast<int>(left), static_cast<int>(right));
    }

    if (number == ScriptNumber::Float) {
        return convert(ScriptNumber::Float, realArithmetic(instruction.opcode, convert(ScriptNumber::Float, left),
                                                           convert(ScriptNumber::Float, right)));
    }

    return realArithmetic(instruction.opcode, left, right);
}

int ScriptRun::arithmetic(ScriptOpcode opcode, int left, int right)
{
    const unsigned int wrappingLeft  = static_cast<unsigned int>(left);
    const unsigned int wrappingRight = static_cast<unsigned int>(right);

    const bool divisorIsRepresentable = right != 0
                                        && !(left == std::numeric_limits<int>::min() && right == -1);

    switch (opcode) {
        case ScriptOpcode::Add:            return static_cast<int>(wrappingLeft + wrappingRight);
        case ScriptOpcode::Subtract:       return static_cast<int>(wrappingLeft - wrappingRight);
        case ScriptOpcode::Negate:         return static_cast<int>(wrappingLeft - wrappingRight);
        case ScriptOpcode::Multiply:       return static_cast<int>(wrappingLeft * wrappingRight);
        case ScriptOpcode::Equal:          return static_cast<int>(left == right);
        case ScriptOpcode::NotEqual:       return static_cast<int>(left != right);
        case ScriptOpcode::Less:           return static_cast<int>(left < right);
        case ScriptOpcode::LessOrEqual:    return static_cast<int>(left <= right);
        case ScriptOpcode::Greater:        return static_cast<int>(left > right);
        case ScriptOpcode::GreaterOrEqual: return static_cast<int>(left >= right);
        case ScriptOpcode::LogicalAnd:     return static_cast<int>(left != 0 && right != 0);
        case ScriptOpcode::LogicalOr:      return static_cast<int>(left != 0 || right != 0);
        case ScriptOpcode::LogicalNot:     return static_cast<int>(right == 0);

        case ScriptOpcode::Divide: {
            if (divisorIsRepresentable) {
                return left / right;
            }

            return 0;
        }

        case ScriptOpcode::Modulo: {
            if (divisorIsRepresentable) {
                return left % right;
            }

            return 0;
        }

        default:
            return 0;
    }
}

double ScriptRun::realArithmetic(ScriptOpcode opcode, double left, double right)
{
    switch (opcode) {
        case ScriptOpcode::Add:            return left + right;
        case ScriptOpcode::Subtract:       return left - right;
        case ScriptOpcode::Negate:         return -right;
        case ScriptOpcode::Multiply:       return left * right;
        case ScriptOpcode::Equal:          return static_cast<double>(left == right);
        case ScriptOpcode::NotEqual:       return static_cast<double>(left != right);
        case ScriptOpcode::Less:           return static_cast<double>(left < right);
        case ScriptOpcode::LessOrEqual:    return static_cast<double>(left <= right);
        case ScriptOpcode::Greater:        return static_cast<double>(left > right);
        case ScriptOpcode::GreaterOrEqual: return static_cast<double>(left >= right);
        case ScriptOpcode::LogicalAnd:     return static_cast<double>(left != 0.0 && right != 0.0);
        case ScriptOpcode::LogicalOr:      return static_cast<double>(left != 0.0 || right != 0.0);
        case ScriptOpcode::LogicalNot:     return static_cast<double>(right == 0.0);

        case ScriptOpcode::Divide: {
            if (right != 0.0) {
                return left / right;
            }

            return 0.0;
        }

        default:
            return 0.0;
    }
}

double ScriptRun::convert(ScriptNumber number, double value)
{
    constexpr double intLowest    = std::numeric_limits<int>::min();
    constexpr double intHighest   = std::numeric_limits<int>::max();
    constexpr double floatHighest = std::numeric_limits<float>::max();

    switch (number) {
        case ScriptNumber::Int: {
            if (std::isnan(value)) {
                return 0.0;
            }

            return std::trunc(std::clamp(value, intLowest, intHighest));
        }

        case ScriptNumber::Float: {
            if (std::isfinite(value)) {
                value = std::clamp(value, -floatHighest, floatHighest);
            }

            return static_cast<float>(value);
        }

        case ScriptNumber::Double:
            return value;
    }

    return value;
}

int ScriptRun::readContext(ScriptContextValue value) const
{
    switch (value) {
        case ScriptContextValue::Current:           return context.currentNodeId;
        case ScriptContextValue::TraversalId:       return context.traversalKey.typeId;
        case ScriptContextValue::TraversalRandom:   return context.randomValue;
        case ScriptContextValue::TraversalInstance: return context.traversalKey.instance;
    }

    return 0;
}

int ScriptRun::readField(ScriptField field, int nodeId)
{
    const RTNode* const node = context.nodes.find(nodeId);

    if (node == nullptr) {
        if (field == ScriptField::Id || field == ScriptField::LastChild || field == ScriptField::Parent) {
            return -1;
        }

        return 0;
    }

    switch (field) {
        case ScriptField::Id:           return nodeId;
        case ScriptField::Pitch:        return noteValue(field, *node);
        case ScriptField::Velocity:     return noteValue(field, *node);
        case ScriptField::Count:        return readState(context.countSlot, nodeId);
        case ScriptField::SwitchCount:  return readState(NodeStateSlot::SwitchCount, nodeId);
        case ScriptField::TriggerCount: return readState(NodeStateSlot::Trigger, nodeId);
        case ScriptField::SubLoopCount: return readState(NodeStateSlot::SubRootCount, nodeId);
        case ScriptField::CountLimit:   return node->countLimit;
        case ScriptField::TriggerLimit: return node->triggerLimit;
        case ScriptField::SwitchLimit:  return node->switchCountLimit;
        case ScriptField::SubLoopLimit: return node->subLoopCountLimit;
        case ScriptField::Repeat:       return node->repeatValue;
        case ScriptField::Probability:  return node->probability;
        case ScriptField::ChildCount:   return static_cast<int>(node->connections.size());
        case ScriptField::LastChild:    return readState(NodeStateSlot::LastNode, nodeId);
        case ScriptField::Eligible:     return 1;

        case ScriptField::Duration: {
            if (context.host == nullptr) {
                return 0;
            }

            return context.host->noteDuration(nodeId);
        }

        case ScriptField::Parent: {
            if (context.nodes.find(node->parentId) == nullptr) {
                return -1;
            }

            return node->parentId;
        }
    }

    return 0;
}

void ScriptRun::writeField(ScriptField field, int nodeId, int value)
{
    if (context.nodes.find(nodeId) == nullptr) {
        return;
    }

    switch (field) {
        case ScriptField::Count:        writeState(context.countSlot, nodeId, value);            break;
        case ScriptField::SwitchCount:  writeState(NodeStateSlot::SwitchCount, nodeId, value);  break;
        case ScriptField::TriggerCount: writeState(NodeStateSlot::Trigger, nodeId, value);      break;
        case ScriptField::SubLoopCount: writeState(NodeStateSlot::SubRootCount, nodeId, value); break;
        default:                                                                                 break;
    }
}

int ScriptRun::readState(NodeStateSlot slot, int nodeId) const
{
    for (int index = trialWriteCount; index > 0; --index) {
        const TrialWrite& write = trialWrites[static_cast<std::size_t>(index - 1)];

        if (write.slot == slot && write.nodeId == nodeId) {
            return write.value;
        }
    }

    return context.nodeState.get(slot, nodeId);
}

void ScriptRun::writeState(NodeStateSlot slot, int nodeId, int value)
{
    if (context.writes == ScriptWrites::Commit) {
        context.nodeState.set(slot, nodeId, value);
        return;
    }

    for (int index = 0; index < trialWriteCount; ++index) {
        TrialWrite& write = trialWrites[static_cast<std::size_t>(index)];

        if (write.slot == slot && write.nodeId == nodeId) {
            write.value = value;
            return;
        }
    }

    if (trialWriteCount < trialWriteCapacity) {
        trialWrites[static_cast<std::size_t>(trialWriteCount++)] = { slot, nodeId, value };
    }
}

int ScriptRun::noteValue(ScriptField field, const RTNode& node) const
{
    const int           alternativeId = readState(NodeStateSlot::ActiveAlternative, node.nodeID);
    const RTNode* const alternative   = context.nodes.find(alternativeId);

    const RTNode* voiced   = &node;
    int           velocity = 0;

    if (node.notes.empty()) {
        return RTNote::fallbackValue;
    }

    if (alternative != nullptr && !alternative->notes.empty()) {
        voiced = alternative;
    }

    if (field == ScriptField::Pitch) {
        return voiced->notes[0].pitch;
    }

    velocity = voiced->notes[0].velocity;

    if (velocity <= 0) {
        velocity = RTNote::fallbackValue;
    }

    return velocity;
}

int ScriptRun::childAt(int parentId, int childIndex) const
{
    const RTNode* const parent = context.nodes.find(parentId);

    if (parent == nullptr || childIndex < 0 || childIndex >= static_cast<int>(parent->connections.size())) {
        return -1;
    }

    const RuleContext ruleContext { context.nodes, *parent, 0, context.traversalKey, context.isEligible,
                                    context.nodeState, context.randomValue };

    const int childId = parent->connections[static_cast<std::size_t>(childIndex)].childId;

    if (ruleContext.eligibleChild(childId) == nullptr) {
        return -1;
    }

    return childId;
}
