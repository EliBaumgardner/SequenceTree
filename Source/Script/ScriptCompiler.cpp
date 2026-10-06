#include "ScriptCompiler.h"

#include "ScriptEmitter.h"
#include "ScriptLexer.h"
#include "ScriptParser.h"

ScriptCompileResult compileTraversalScript(const std::string& source)
{
    ScriptCompileResult result;

    Lexer lexer(source, result.diagnostics);

    const std::vector<Token> tokens = lexer.run();

    if (!result.diagnostics.empty()) {
        return result;
    }

    Parser parser(tokens, result.diagnostics);

    const ClassDeclaration declaration = parser.run();

    if (!result.diagnostics.empty()) {
        return result;
    }

    Emitter emitter(result.script, result.diagnostics);

    emitter.run(declaration);

    if (!result.diagnostics.empty()) {
        result.script = RTScript {};
    }

    return result;
}

const char* defaultTraversalScriptSource()
{
    return
        "import core;\n"
        "\n"
        "// main runs each time a note ends. It moves\n"
        "// the walk one node and plays that node as\n"
        "// it is written.\n"
        "\n"
        "class Traversal : defaultTraversal {\n"
        "\n"
        "public:\n"
        "\n"
        "    Node selectedNode;\n"
        "\n"
        "    void main() {\n"
        "        selectedNode = advance(1);\n"
        "\n"
        "        int pitch    = selectedNode.pitch;\n"
        "        int duration = selectedNode.duration;\n"
        "        int velocity = selectedNode.velocity;\n"
        "\n"
        "        playNote(selectedNode, pitch, duration, velocity);\n"
        "    }\n"
        "\n"
        "};\n";
}
