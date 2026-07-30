#pragma once

#include "RTScript.h"
#include "TraversalRule.h"

class ScriptTraversalRule : public TraversalRule
{
public:

    void setScript(const RTScript* newScript) { script = newScript; }

    const RTScript* getScript() const { return script; }

    int selectChild(const RuleContext& context) const override;

private:

    const RTScript* script = nullptr;
};
