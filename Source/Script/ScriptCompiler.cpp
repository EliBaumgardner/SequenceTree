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
        "// This script picks the child the traversal\n"
        "// moves to after a node plays. It runs once\n"
        "// per step, and returns a child id, or none\n"
        "// to let the traversal stop at this node.\n"
        "//\n"
        "// The graph is read through fixed names.\n"
        "//\n"
        "// parent.id          the node that just\n"
        "//                    played\n"
        "// parent.count       times this traversal\n"
        "//                    has visited that node\n"
        "// parent.childCount  arrows leaving it\n"
        "// parent.lastChild   child picked here on\n"
        "//                    the previous visit\n"
        "// children.count     same as parent.childCount\n"
        "//\n"
        "// for child in children { }  walks every\n"
        "// arrow leaving the parent. The loop name\n"
        "// is yours to choose, and these loops\n"
        "// cannot be nested.\n"
        "//\n"
        "// child.id            the child node id\n"
        "// child.eligible      true when this arrow\n"
        "//                     may be taken at all\n"
        "// child.limit         count limit\n"
        "// child.triggerLimit  trigger limit, 0 off\n"
        "// child.triggerCount  triggers so far\n"
        "// child.visits        visits so far\n"
        "// child.repeat        repeat value\n"
        "// child.pitch         arrow pitch offset\n"
        "// child.switchLimit   switch count limit\n"
        "// child.subLoopLimit  sub loop count limit\n"
        "// child.probability   weight, 100 is full\n"
        "//\n"
        "// An arrow is eligible when it has length,\n"
        "// its count limit is above zero, its\n"
        "// trigger limit is unspent, and it is not\n"
        "// disabled for this traversal. Every other\n"
        "// property of an ineligible child reads 0,\n"
        "// and its id reads -1.\n"
        "//\n"
        "// traversal.id        which traversal is\n"
        "//                     walking the graph\n"
        "// traversal.instance  which copy of it\n"
        "// traversal.random    a fresh number, one\n"
        "//                     per step\n"
        "//\n"
        "// The language has let, if, else, for in,\n"
        "// while, break, continue, return and none,\n"
        "// with and, or, not for logic. Statements\n"
        "// end with a semicolon, and a line beginning\n"
        "// with two slashes is a note like this one.\n"
        "//\n"
        "// Press play above to make the rule you are\n"
        "// viewing the live one. The bar below shows\n"
        "// its compiled size, or the first error.\n"
        "//\n"
        "// Everything past here is the built in rule.\n"
        "\n"
        "// A child is due on this visit when its count\n"
        "// limit divides parent.count evenly. Where\n"
        "// several are due, the largest limit wins, so\n"
        "// the slowest arrow takes the step.\n"
        "\n"
        "let maxLimit = 0;\n"
        "\n"
        "for child in children {\n"
        "    if !child.eligible { continue; }\n"
        "\n"
        "    if parent.count % child.limit == 0\n"
        "       && child.limit > maxLimit {\n"
        "        maxLimit = child.limit;\n"
        "    }\n"
        "}\n"
        "\n"
        "// Add up the weight of the children tied at\n"
        "// that limit. They are the candidates.\n"
        "\n"
        "let totalWeight = 0;\n"
        "\n"
        "for child in children {\n"
        "    if !child.eligible { continue; }\n"
        "\n"
        "    if parent.count % child.limit == 0\n"
        "       && child.limit == maxLimit {\n"
        "        totalWeight += child.probability;\n"
        "    }\n"
        "}\n"
        "\n"
        "// Nothing is due, so the traversal stops.\n"
        "\n"
        "if totalWeight <= 0 { return none; }\n"
        "\n"
        "// The span never drops below 100, so weights\n"
        "// summing under 100 leave a share of the\n"
        "// range spare and no child is picked at all.\n"
        "\n"
        "let selectionSpan = totalWeight;\n"
        "\n"
        "if selectionSpan < 100 { selectionSpan = 100; }\n"
        "\n"
        "// Land the random pick somewhere in that span\n"
        "// and take the candidate whose weight covers\n"
        "// it, which favours the heavier children.\n"
        "\n"
        "let pick = traversal.random % selectionSpan;\n"
        "let runningWeight = 0;\n"
        "let chosen = none;\n"
        "\n"
        "for child in children {\n"
        "    if !child.eligible { continue; }\n"
        "\n"
        "    if parent.count % child.limit == 0\n"
        "       && child.limit == maxLimit {\n"
        "        runningWeight += child.probability;\n"
        "\n"
        "        if pick < runningWeight {\n"
        "            chosen = child.id;\n"
        "            break;\n"
        "        }\n"
        "    }\n"
        "}\n"
        "\n"
        "return chosen;\n";
}
