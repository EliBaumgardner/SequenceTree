#pragma once

#include <string>
#include <vector>

#include "../Audio/RTScript.h"

struct ScriptDiagnostic
{
    std::string message;
    int         line   = 1;
    int         column = 1;
    int         length = 1;
};

struct ScriptCompileResult
{
    RTScript                      script;
    std::vector<ScriptDiagnostic> diagnostics;

    bool succeeded() const { return diagnostics.empty(); }
};

ScriptCompileResult compileTraversalScript(const std::string& source);

const char* defaultTraversalScriptSource();
