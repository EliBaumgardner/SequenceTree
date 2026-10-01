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
        "// This class runs the traversal. main runs\n"
        "// each time a note ends: it moves the walk\n"
        "// and decides what is heard next.\n"
        "//\n"
        "// advance(numSteps) moves the walk that many\n"
        "// nodes and gives back the node it lands on,\n"
        "// or none when the walk has ended. Nodes it\n"
        "// passes over are skipped entirely.\n"
        "//\n"
        "// playNote(node, pitch, duration, velocity)\n"
        "// plays the next note. If main never calls\n"
        "// it, nothing is heard for that step, and\n"
        "// the walk still carries on.\n"
        "//\n"
        "// Write your own Node advance(int numSteps)\n"
        "// to take over how the walk moves. It then\n"
        "// owns counting, switch counts and sub loops,\n"
        "// and returns the node to land on, or none.\n"
        "// Without one, the built in walk is used.\n"
        "//\n"
        "// current       the node the walk is on\n"
        "// for child in children { }  walks the arrows\n"
        "// leaving current; node.children walks those\n"
        "// of any node. A child that cannot be taken\n"
        "// reads as none.\n"
        "//\n"
        "// A Node has these properties.\n"
        "//\n"
        "// id, pitch, velocity, duration\n"
        "// count, switchCount, triggerCount,\n"
        "// subLoopCount        these can be changed\n"
        "// limit, triggerLimit, switchLimit,\n"
        "// subLoopLimit, repeat, probability,\n"
        "// childCount, eligible\n"
        "// lastChild, parent   these are Nodes\n"
        "//\n"
        "// traversal.id, traversal.instance and\n"
        "// traversal.random, a fresh number per step.\n"
        "//\n"
        "// Members keep their value from one step to\n"
        "// the next. A number starts at 0 and a Node\n"
        "// at none.\n"
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
