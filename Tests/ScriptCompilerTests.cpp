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
