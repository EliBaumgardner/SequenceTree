#pragma once

#include <string>
#include <vector>

#include "RTScript.h"

/*
    The public face of the traversal scripting language: one function that turns a script's source
    text into the RTScript the audio thread runs. compileTraversalScript runs the three phases in
    order, Lexer (text into tokens), Parser (tokens into a syntax tree) and Emitter (tree into
    bytecode), and stops after the first phase that reports an error. Each phase is a local of that
    call, so nothing carries over from one compile to the next.

    The result is the RTScript, or a list of ScriptDiagnostics, each with a message and the line,
    column and length of the text to mark; a failed compile leaves the script empty.
    defaultTraversalScriptSource is the script a new rule starts with, which plays the way the
    built-in rule does.

    It runs on the message thread only. AudioSnapshotPublisher compiles the active rule with it,
    and TraversalRulesWindow compiles the text being edited to show the diagnostics;
    TraversalRuleState uses the default source.
*/

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
