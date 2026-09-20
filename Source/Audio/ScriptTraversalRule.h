#pragma once

#include "../Script/RTScript.h"
#include "TraversalRule.h"

class ScriptTraversalRule : public TraversalRule
{
public:

    int selectChild(const RuleContext& context) const override;

    const RTScript* script = nullptr;
};
