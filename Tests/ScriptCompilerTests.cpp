#include <catch2/catch_test_macros.hpp>

#include "Audio/ScriptTraversalRule.h"
#include "Script/ScriptCompiler.h"

#include <memory>
#include <string>

static int runScript(const std::string& source)
{
    const ScriptCompileResult compiled = compileTraversalScript(source);

    REQUIRE(compiled.succeeded());

    RTConnection connection;
    connection.childId  = 2;
    connection.duration = 500;

    RTNode parent;
    parent.nodeID = 1;
    parent.connections.push_back(connection);

    RTNode child;
    child.nodeID     = 2;
    child.parentId   = 1;
    child.countLimit = 1;

    NodeMap nodes;
    nodes[1] = std::make_shared<const RTNode>(parent);
    nodes[2] = std::make_shared<const RTNode>(child);

    NodeStateTable nodeState;
    nodeState.prepare();

    const RuleContext context { nodes, parent, 1, TraversalKey {},
                                [] (RTNode::NodeType) { return true; }, nodeState, 0 };

    ScriptTraversalRule rule;
    rule.script = &compiled.script;

    return rule.selectChild(context);
}

TEST_CASE("the default script compiles cleanly", "[script]")
{
    const ScriptCompileResult compiled = compileTraversalScript(defaultTraversalScriptSource());

    CHECK(compiled.succeeded());
    CHECK_FALSE(compiled.script.isEmpty());
}

TEST_CASE("a syntax error is reported on the line it occurs", "[script]")
{
    const ScriptCompileResult compiled = compileTraversalScript("let a = 1;\n\nlet b = ;\n");

    REQUIRE(compiled.diagnostics.size() == 1);
    CHECK(compiled.diagnostics.front().line == 3);
    CHECK(compiled.script.isEmpty());
}

TEST_CASE("an unknown field is rejected", "[script]")
{
    const ScriptCompileResult compiled = compileTraversalScript("return parent.bogus;");

    CHECK_FALSE(compiled.succeeded());
}

TEST_CASE("a newline does not end a statement", "[script]")
{
    CHECK(runScript("let a =\n    2;\nreturn a;") == 2);
}

TEST_CASE("the script returns the child it chooses", "[script]")
{
    CHECK(runScript("for child in children { if child.eligible { return child.id; } } return -1;") == 2);
}

TEST_CASE("a script that never returns is stopped by the step budget", "[script]")
{
    CHECK(runScript("while 1 == 1 { } return 2;") == -1);
}

TEST_CASE("while, break and continue steer a loop", "[script]")
{
    CHECK(runScript("let total = 0;\n"
                    "let i = 0;\n"
                    "while i < 10 {\n"
                    "    i += 1;\n"
                    "    if i % 2 == 0 { continue; }\n"
                    "    if i > 7 { break; }\n"
                    "    total += i;\n"
                    "}\n"
                    "return total;") == 16);
}

TEST_CASE("break and continue work inside a child loop", "[script]")
{
    CHECK(runScript("let seen = 0; for c in children { seen += c.id; break; } return seen;") == 2);
    CHECK(runScript("let seen = 0; for c in children { continue; seen = 9; } return seen;") == 0);
}

TEST_CASE("and binds tighter than or, and not applies to one operand", "[script]")
{
    CHECK(runScript("return (1 == 1 and not 0) + (0 or 0) * 10 + (not (2 < 1 or 1 > 2)) * 100;") == 101);
    CHECK(runScript("return 1 or 0 and 0;") == 1);
    CHECK(runScript("return (1 && !0) + (0 || 1) * 10;") == 11);
}

TEST_CASE("arithmetic follows precedence and a zero divisor reads 0", "[script]")
{
    CHECK(runScript("return 2 + 3 * 4 - -1;") == 15);
    CHECK(runScript("return 7 / 0 + 7 % 0 + 1;") == 1);
    CHECK(runScript("return -(3 - 5);") == 2);
}

TEST_CASE("a comment ends at a closing slash pair or at the end of the line", "[script]")
{
    CHECK(runScript("let a = 1; // closed // let b = 2;\n// return 9;\nreturn a + b;") == 3);
}

TEST_CASE("a variable declared in a block is gone after it", "[script]")
{
    const ScriptCompileResult compiled = compileTraversalScript("if 1 == 1 { let a = 5; }\nreturn a;");

    REQUIRE(compiled.diagnostics.size() == 1);
    CHECK(compiled.diagnostics.front().line == 2);
}

TEST_CASE("malformed scripts are rejected", "[script]")
{
    for (const char* source : { "x = 1; return x;",
                                "break;",
                                "continue;",
                                "for a in children { for b in children { } }",
                                "for a in parent { }",
                                "for a in children { let a = 1; }",
                                "return parent;",
                                "let a = 1; return a.count;",
                                "return 1 $ 2;",
                                "return 99999999999;" }) {
        CAPTURE(source);
        CHECK_FALSE(compileTraversalScript(source).succeeded());
    }
}

TEST_CASE("a diagnostic marks the whole offending expression", "[script]")
{
    const ScriptCompileResult compiled = compileTraversalScript("let a = 1;\nreturn parent.bogus;");

    REQUIRE(compiled.diagnostics.size() == 1);
    CHECK(compiled.diagnostics.front().line   == 2);
    CHECK(compiled.diagnostics.front().column == 8);
    CHECK(compiled.diagnostics.front().length == 12);
}

TEST_CASE("the compiler refuses scripts that would overflow the traversal stack", "[script]")
{
    std::string manyLocals;

    for (int local = 0; local <= RTScript::maxLocals; ++local) {
        manyLocals += "let v" + std::to_string(local) + " = 1;\n";
    }

    CHECK_FALSE(compileTraversalScript(manyLocals + "return 1;").succeeded());

    std::string deepExpression = "1";

    for (int depth = 0; depth <= RTScript::maxStack; ++depth) {
        deepExpression = "1 + (" + deepExpression + ")";
    }

    CHECK_FALSE(compileTraversalScript("return " + deepExpression + ";").succeeded());

    std::string fittingLocals;

    for (int local = 0; local < RTScript::maxLocals; ++local) {
        fittingLocals += "let v" + std::to_string(local) + " = 1;\n";
    }

    CHECK(compileTraversalScript(fittingLocals + "return 1;").succeeded());
}
