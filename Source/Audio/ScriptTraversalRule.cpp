#include "ScriptTraversalRule.h"

#include <cstddef>

namespace {

int readScriptField(const RuleContext& context, ScriptField field,
                    const RTNode* currentChild, int currentChildId)
{
    switch (field) {
        case ScriptField::ParentId:
            return context.parent.nodeID;

        case ScriptField::ParentCount:
            return context.parentCount;

        case ScriptField::ParentChildCount:
            return static_cast<int>(context.parent.children.size());

        case ScriptField::ParentLastChosenChild:
            return context.nodeState.get(NodeStateSlot::LastNode, context.parent.nodeID);

        case ScriptField::TraversalId:
            return context.traversalId;

        case ScriptField::ChildIsEligible:
            return currentChild != nullptr ? 1 : 0;

        default:
            break;
    }

    if (currentChild == nullptr) {
        return field == ScriptField::ChildId ? -1 : 0;
    }

    switch (field) {
        case ScriptField::ChildId:
            return currentChildId;

        case ScriptField::ChildCountLimit:
            return currentChild->countLimit;

        case ScriptField::ChildTriggerLimit:
            return currentChild->triggerLimit;

        case ScriptField::ChildTriggerCount:
            return context.nodeState.get(NodeStateSlot::Trigger, currentChildId);

        case ScriptField::ChildVisitCount:
            return context.nodeState.get(NodeStateSlot::Count, currentChildId);

        case ScriptField::ChildRepeatValue:
            return currentChild->repeatValue;

        case ScriptField::ChildPitchOffset:
            return currentChild->pitchOffset;

        case ScriptField::ChildSwitchCountLimit:
            return currentChild->switchCountLimit;

        case ScriptField::ChildSubLoopCountLimit:
            return currentChild->subLoopCountLimit;

        default:
            return 0;
    }
}

}

int ScriptTraversalRule::selectChild(const RuleContext& context) const
{
    if (script == nullptr || script->isEmpty()) {
        return -1;
    }

    int stack[RTScript::maxStack];
    int stackTop = 0;

    int locals[RTScript::maxLocals];

    for (int localIndex = 0; localIndex < RTScript::maxLocals; ++localIndex) {
        locals[localIndex] = 0;
    }

    const RTNode* currentChild   = nullptr;
    int           currentChildId = -1;

    const int instructionCount = static_cast<int>(script->instructions.size());

    int programCounter = 0;
    int stepsRemaining = script->stepBudget;

    while (programCounter >= 0 && programCounter < instructionCount) {

        if (--stepsRemaining < 0) {
            return -1;
        }

        const ScriptInstruction instruction = script->instructions[programCounter];

        ++programCounter;

        switch (instruction.opcode) {

            case ScriptOpcode::Halt: {
                return -1;
            }

            case ScriptOpcode::PushInt: {
                if (stackTop >= RTScript::maxStack) {
                    return -1;
                }

                stack[stackTop++] = instruction.operand;
                break;
            }

            case ScriptOpcode::PushLocal: {
                if (stackTop >= RTScript::maxStack) {
                    return -1;
                }

                if (instruction.operand < 0 || instruction.operand >= RTScript::maxLocals) {
                    return -1;
                }

                stack[stackTop++] = locals[instruction.operand];
                break;
            }

            case ScriptOpcode::PushField: {
                if (stackTop >= RTScript::maxStack) {
                    return -1;
                }

                const ScriptField field = static_cast<ScriptField>(instruction.operand);

                stack[stackTop++] = readScriptField(context, field, currentChild, currentChildId);
                break;
            }

            case ScriptOpcode::StoreLocal: {
                if (stackTop < 1) {
                    return -1;
                }

                if (instruction.operand < 0 || instruction.operand >= RTScript::maxLocals) {
                    return -1;
                }

                locals[instruction.operand] = stack[--stackTop];
                break;
            }

            case ScriptOpcode::Pop: {
                if (stackTop < 1) {
                    return -1;
                }

                --stackTop;
                break;
            }

            case ScriptOpcode::LoadChild: {
                if (stackTop < 1) {
                    return -1;
                }

                const int childIndex = stack[--stackTop];
                const int childCount = static_cast<int>(context.parent.children.size());

                if (childIndex < 0 || childIndex >= childCount) {
                    currentChild   = nullptr;
                    currentChildId = -1;
                    break;
                }

                currentChildId = context.parent.children[static_cast<std::size_t>(childIndex)];
                currentChild   = context.eligibleChild(currentChildId);
                break;
            }

            case ScriptOpcode::Negate:
            case ScriptOpcode::LogicalNot: {
                if (stackTop < 1) {
                    return -1;
                }

                const int value = stack[stackTop - 1];

                stack[stackTop - 1] = instruction.opcode == ScriptOpcode::Negate ? -value
                                                                                 : (value == 0 ? 1 : 0);
                break;
            }

            case ScriptOpcode::Add:
            case ScriptOpcode::Subtract:
            case ScriptOpcode::Multiply:
            case ScriptOpcode::Divide:
            case ScriptOpcode::Modulo:
            case ScriptOpcode::Equal:
            case ScriptOpcode::NotEqual:
            case ScriptOpcode::Less:
            case ScriptOpcode::LessOrEqual:
            case ScriptOpcode::Greater:
            case ScriptOpcode::GreaterOrEqual:
            case ScriptOpcode::LogicalAnd:
            case ScriptOpcode::LogicalOr: {
                if (stackTop < 2) {
                    return -1;
                }

                const int right = stack[--stackTop];
                const int left  = stack[--stackTop];

                int result = 0;

                switch (instruction.opcode) {
                    case ScriptOpcode::Add:            result = left + right;                     break;
                    case ScriptOpcode::Subtract:       result = left - right;                     break;
                    case ScriptOpcode::Multiply:       result = left * right;                     break;
                    case ScriptOpcode::Divide:         result = right != 0 ? left / right : 0;    break;
                    case ScriptOpcode::Modulo:         result = right != 0 ? left % right : 0;    break;
                    case ScriptOpcode::Equal:          result = left == right ? 1 : 0;            break;
                    case ScriptOpcode::NotEqual:       result = left != right ? 1 : 0;            break;
                    case ScriptOpcode::Less:           result = left <  right ? 1 : 0;            break;
                    case ScriptOpcode::LessOrEqual:    result = left <= right ? 1 : 0;            break;
                    case ScriptOpcode::Greater:        result = left >  right ? 1 : 0;            break;
                    case ScriptOpcode::GreaterOrEqual: result = left >= right ? 1 : 0;            break;
                    case ScriptOpcode::LogicalAnd:     result = (left != 0 && right != 0) ? 1 : 0; break;
                    case ScriptOpcode::LogicalOr:      result = (left != 0 || right != 0) ? 1 : 0; break;
                    default:                                                                      break;
                }

                stack[stackTop++] = result;
                break;
            }

            case ScriptOpcode::Jump: {
                programCounter = instruction.operand;
                break;
            }

            case ScriptOpcode::JumpIfFalse: {
                if (stackTop < 1) {
                    return -1;
                }

                if (stack[--stackTop] == 0) {
                    programCounter = instruction.operand;
                }
                break;
            }

            case ScriptOpcode::JumpIfTrue: {
                if (stackTop < 1) {
                    return -1;
                }

                if (stack[--stackTop] != 0) {
                    programCounter = instruction.operand;
                }
                break;
            }

            case ScriptOpcode::Return: {
                if (stackTop < 1) {
                    return -1;
                }

                return stack[--stackTop];
            }
        }
    }

    return -1;
}
