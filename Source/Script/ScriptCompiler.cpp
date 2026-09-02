#include "ScriptCompiler.h"

#include "ScriptEmitter.h"
#include "ScriptLexer.h"
#include "ScriptParser.h"

ScriptCompileResult compileTraversalScript(const std::string& source)
{
    ScriptCompileResult result;

    script::Lexer lexer(source, result.diagnostics);

    const std::vector<script::Token> tokens = lexer.run();

    if (!result.diagnostics.empty()) {
        return result;
    }

    script::Parser parser(tokens, result.diagnostics);

    const std::vector<script::StatementPtr> program = parser.run();

    if (!result.diagnostics.empty()) {
        return result;
    }

    script::Emitter emitter(result.script, result.diagnostics);

    emitter.run(program);

    if (!result.diagnostics.empty()) {
        result.script.instructions.clear();
        result.script.localCount = 0;
    }

    return result;
}

const char* defaultTraversalScriptSource()
{
    return
        "let chosen = none;\n"
        "let maxLimit = 0;\n"
        "\n"
        "for child in children {\n"
        "    if !child.eligible { continue; }\n"
        "\n"
        "    if parent.count % child.limit == 0\n"
        "       && child.limit > maxLimit {\n"
        "        chosen = child.id;\n"
        "        maxLimit = child.limit;\n"
        "    }\n"
        "}\n"
        "\n"
        "return chosen;\n";
}
