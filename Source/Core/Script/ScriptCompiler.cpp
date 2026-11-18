#include "ScriptCompiler.h"

#include "ScriptEmitter.h"
#include "ScriptLexer.h"
#include "ScriptParser.h"

#include <algorithm>
#include <atomic>

static std::string typeName(ValueType type)
{
    switch (type) {
        case ValueType::Int:      return "int";
        case ValueType::Float:    return "float";
        case ValueType::Double:   return "double";
        case ValueType::Bool:     return "bool";
        case ValueType::Node:     return "Node";
        case ValueType::Void:     return "void";
        case ValueType::Inferred: return "let";
    }

    return {};
}

static ScriptOutline makeOutline(const ClassDeclaration& declaration)
{
    ScriptOutline outline;

    outline.className = declaration.name;

    for (const MemberDeclaration& global : declaration.globals) {
        outline.entries.push_back({ ScriptOutlineEntry::Kind::Global, global.name, typeName(global.type), global.line });
    }

    for (const MemberDeclaration& member : declaration.members) {
        outline.entries.push_back({ ScriptOutlineEntry::Kind::Member, member.name, typeName(member.type), member.line });
    }

    for (const FunctionDeclaration& function : declaration.functions) {
        std::string parameters;

        for (const Parameter& parameter : function.parameters) {
            if (! parameters.empty()) {
                parameters += ", ";
            }

            parameters += typeName(parameter.type);
        }

        outline.entries.push_back({ ScriptOutlineEntry::Kind::Function, function.name + "(" + parameters + ")", typeName(function.returnType), function.line });
    }

    std::ranges::stable_sort(outline.entries, {}, &ScriptOutlineEntry::line);

    return outline;
}

static std::atomic<std::uint64_t> compileCount { 0 };

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
    result.outline                     = makeOutline(declaration);

    if (!result.diagnostics.empty()) {
        return result;
    }

    Emitter emitter(result.script, result.diagnostics);
    emitter.run(declaration);

    if (!result.diagnostics.empty()) {
        result.script = RTScript {};

        return result;
    }

    result.script.generation = ++compileCount;

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
