#include "RTScript.h"

RTScript makeNativeSelectChildScript()
{
    constexpr int chosenLocal   = 0;
    constexpr int maxLimitLocal = 1;
    constexpr int indexLocal    = 2;

    RTScript script;

    script.localCount = 3;

    std::vector<ScriptInstruction>& code = script.instructions;

    code.push_back({ ScriptOpcode::PushInt, -1 });
    code.push_back({ ScriptOpcode::StoreLocal, chosenLocal });

    code.push_back({ ScriptOpcode::PushInt, 0 });
    code.push_back({ ScriptOpcode::StoreLocal, maxLimitLocal });

    code.push_back({ ScriptOpcode::PushInt, 0 });
    code.push_back({ ScriptOpcode::StoreLocal, indexLocal });

    const int loopTop = static_cast<int>(code.size());

    code.push_back({ ScriptOpcode::PushLocal, indexLocal });
    code.push_back({ ScriptOpcode::PushField, ScriptField::ParentChildCount });
    code.push_back({ ScriptOpcode::Less });

    const int exitLoopJump = static_cast<int>(code.size());
    code.push_back({ ScriptOpcode::JumpIfFalse, 0 });

    code.push_back({ ScriptOpcode::PushLocal, indexLocal });
    code.push_back({ ScriptOpcode::LoadChild });

    code.push_back({ ScriptOpcode::PushField, ScriptField::ChildIsEligible });

    const int skipIneligibleJump = static_cast<int>(code.size());
    code.push_back({ ScriptOpcode::JumpIfFalse, 0 });

    code.push_back({ ScriptOpcode::PushField, ScriptField::ParentCount });
    code.push_back({ ScriptOpcode::PushField, ScriptField::ChildCountLimit });
    code.push_back({ ScriptOpcode::Modulo });
    code.push_back({ ScriptOpcode::PushInt, 0 });
    code.push_back({ ScriptOpcode::Equal });

    const int skipOffBeatJump = static_cast<int>(code.size());
    code.push_back({ ScriptOpcode::JumpIfFalse, 0 });

    code.push_back({ ScriptOpcode::PushField, ScriptField::ChildCountLimit });
    code.push_back({ ScriptOpcode::PushLocal, maxLimitLocal });
    code.push_back({ ScriptOpcode::Greater });

    const int skipSmallerLimitJump = static_cast<int>(code.size());
    code.push_back({ ScriptOpcode::JumpIfFalse, 0 });

    code.push_back({ ScriptOpcode::PushField, ScriptField::ChildId });
    code.push_back({ ScriptOpcode::StoreLocal, chosenLocal });

    code.push_back({ ScriptOpcode::PushField, ScriptField::ChildCountLimit });
    code.push_back({ ScriptOpcode::StoreLocal, maxLimitLocal });

    const int nextChild = static_cast<int>(code.size());

    code.push_back({ ScriptOpcode::PushLocal, indexLocal });
    code.push_back({ ScriptOpcode::PushInt, 1 });
    code.push_back({ ScriptOpcode::Add });
    code.push_back({ ScriptOpcode::StoreLocal, indexLocal });
    code.push_back({ ScriptOpcode::Jump, loopTop });

    const int loopExit = static_cast<int>(code.size());

    code.push_back({ ScriptOpcode::PushLocal, chosenLocal });
    code.push_back({ ScriptOpcode::Return });

    code[exitLoopJump].operand          = loopExit;
    code[skipIneligibleJump].operand    = nextChild;
    code[skipOffBeatJump].operand       = nextChild;
    code[skipSmallerLimitJump].operand  = nextChild;

    return script;
}
