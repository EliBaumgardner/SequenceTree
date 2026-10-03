#include <catch2/catch_test_macros.hpp>

#include "Audio/ScriptRun.h"
#include "Script/ScriptCompiler.h"

#include <string>
#include <vector>

static std::string classWith(const std::string& body)
{
    return "class Traversal : defaultTraversal {\npublic:\n" + body + "\n};\n";
}

static NodeMap twoNodeGraph()
{
    RTConnection connection;
    connection.childId  = 2;
    connection.duration = 500;

    RTNote note;
    note.pitch    = 60;
    note.velocity = 100;

    RTNode parent;
    parent.nodeID = 1;
    parent.connections.push_back(connection);

    RTNode child;
    child.nodeID     = 2;
    child.parentId   = 1;
    child.countLimit = 1;
    child.notes.push_back(note);

    return NodeMap { { parent, child } };
}

struct ScriptFixture
{
    NodeMap        nodes = twoNodeGraph();
    NodeStateTable nodeState;
    ScriptMembers  members {};

    ScriptFixture() { nodeState.prepare(); }

    ScriptRunContext context(const RTScript& script, ScriptWrites writes, ScriptHost* host)
    {
        return { script, nodes, nodeState, members, TraversalKey {}, 0, 1, NodeStateSlot::Count,
                 [] (RTNode::NodeType) { return true; }, writes, host };
    }
};

static int runFirstFunction(const std::string& body)
{
    const ScriptCompileResult compiled = compileTraversalScript(classWith(body));

    REQUIRE(compiled.succeeded());

    ScriptFixture fixture;
    ScriptRun     run(fixture.context(compiled.script, ScriptWrites::Commit, nullptr));

    return run.call(0, {});
}

class RecordingHost : public ScriptHost
{
public:

    int advance(int steps) override
    {
        advancedSteps.push_back(steps);
        return 2;
    }

    void playNote(const ScriptNote& note) override
    {
        played.push_back(note);
    }

    int noteDuration(int nodeId) override
    {
        return 480 + nodeId;
    }

    std::vector<int>        advancedSteps;
    std::vector<ScriptNote> played;
};

TEST_CASE("the default script compiles to a main and leaves advance built in", "[script]")
{
    const ScriptCompileResult compiled = compileTraversalScript(defaultTraversalScriptSource());

    REQUIRE(compiled.succeeded());
    CHECK(compiled.script.mainFunction != -1);
    CHECK(compiled.script.advanceFunction == -1);
    CHECK(compiled.script.memberDefaults == std::vector<double> { -1 });
}

TEST_CASE("the class format from the request compiles once its member has a type", "[script]")
{
    const std::string source =
        "class Traversal : defaultTraversal {\n"
        "\n"
        "public:\n"
        "\n"
        "Node selectedNode;\n"
        "\n"
        "void main() {\n"
        "    selectedNode = advance(1);\n"
        "    int pitch      = selectedNode.pitch;\n"
        "    int duration = selectedNode.duration;\n"
        "    int velocity  = selectedNode.velocity;\n"
        "    velocity += 10;\n"
        "    playNote(selectedNode,pitch,duration, velocity);\n"
        "}\n"
        "\n"
        "Node advance(int numSteps) {\n"
        "    for child in children { if child.eligible { return child; } }\n"
        "    return none;\n"
        "}\n"
        "\n"
        "};\n";

    const ScriptCompileResult compiled = compileTraversalScript(source);

    REQUIRE(compiled.succeeded());
    CHECK(compiled.script.mainFunction == 0);
    CHECK(compiled.script.advanceFunction == 1);
}

TEST_CASE("main reads the node advance lands on and hands playNote its values", "[script]")
{
    const ScriptCompileResult compiled = compileTraversalScript(classWith(
        "Node selectedNode;\n"
        "void main() {\n"
        "    selectedNode = advance(3);\n"
        "    int velocity = selectedNode.velocity;\n"
        "    velocity += 10;\n"
        "    playNote(selectedNode, selectedNode.pitch - 12, selectedNode.duration, velocity);\n"
        "}\n"));

    REQUIRE(compiled.succeeded());

    ScriptFixture fixture;
    RecordingHost host;
    ScriptRun     run(fixture.context(compiled.script, ScriptWrites::Commit, &host));

    run.call(compiled.script.mainFunction, {});

    REQUIRE(host.played.size() == 1);
    CHECK(host.advancedSteps == std::vector<int> { 3 });
    CHECK(host.played.front().nodeId == 2);
    CHECK(host.played.front().pitch == 48);
    CHECK(host.played.front().duration == 482);
    CHECK(host.played.front().velocity == 110);
    CHECK(fixture.members[0] == 2);
}

TEST_CASE("a member typed by name alone is reported with the type it needs", "[script]")
{
    const ScriptCompileResult compiled = compileTraversalScript(classWith("selectedNode;"));

    REQUIRE(compiled.diagnostics.size() == 1);
    CHECK(compiled.diagnostics.front().line == 3);
    CHECK(compiled.diagnostics.front().message.find("Node selectedNode;") != std::string::npos);
}

TEST_CASE("a statement without a semicolon is reported where the next one starts", "[script]")
{
    const ScriptCompileResult compiled = compileTraversalScript(classWith(
        "void main() {\n"
        "    int pitch = current.pitch\n"
        "    int duration = 1;\n"
        "}\n"));

    REQUIRE(compiled.diagnostics.size() == 1);
    CHECK(compiled.diagnostics.front().line == 5);
    CHECK(compiled.script.isEmpty());
}

TEST_CASE("a script that is not a class is rejected", "[script]")
{
    CHECK_FALSE(compileTraversalScript("let a = 1; return a;").succeeded());
}

TEST_CASE("a newline does not end a statement", "[script]")
{
    CHECK(runFirstFunction("int value() { int a =\n    2;\nreturn a; }") == 2);
}

TEST_CASE("advance returns the child it chooses", "[script]")
{
    const ScriptCompileResult compiled = compileTraversalScript(classWith(
        "Node advance(int numSteps) { for child in children { if child.eligible { return child; } } return none; }"));

    REQUIRE(compiled.succeeded());

    ScriptFixture fixture;
    ScriptRun     run(fixture.context(compiled.script, ScriptWrites::Commit, nullptr));

    const int arguments[] = { 1 };

    CHECK(run.call(compiled.script.advanceFunction, arguments) == 2);
}

TEST_CASE("child loops nest over any node's children", "[script]")
{
    CHECK(runFirstFunction("int value() {\n"
                           "    int seen = 0;\n"
                           "    for a in children { for b in current.children { seen += a.id * 10 + b.id; } }\n"
                           "    return seen;\n"
                           "}") == 22);
}

TEST_CASE("a script that never returns is stopped by the step budget", "[script]")
{
    CHECK(runFirstFunction("int value() { while 1 == 1 { } return 2; }") == -1);
}

TEST_CASE("endless recursion is stopped by the call depth", "[script]")
{
    CHECK(runFirstFunction("int value() { return value() + 1; }") == -1);
}

TEST_CASE("functions take parameters and return values", "[script]")
{
    CHECK(runFirstFunction("int value() { return twice(4) + twice(1); }\n"
                           "int twice(int amount) { return amount * 2; }") == 10);
}

TEST_CASE("while, break and continue steer a loop", "[script]")
{
    CHECK(runFirstFunction("int value() {\n"
                           "    int total = 0;\n"
                           "    int i = 0;\n"
                           "    while i < 10 {\n"
                           "        i += 1;\n"
                           "        if i % 2 == 0 { continue; }\n"
                           "        if i > 7 { break; }\n"
                           "        total += i;\n"
                           "    }\n"
                           "    return total;\n"
                           "}") == 16);
}

TEST_CASE("and binds tighter than or, and not applies to one operand", "[script]")
{
    CHECK(runFirstFunction("int value() { return (1 == 1 and not 0) + (0 or 0) * 10 + (not (2 < 1 or 1 > 2)) * 100; }")
          == 101);
    CHECK(runFirstFunction("int value() { return 1 or 0 and 0; }") == 1);
}

TEST_CASE("arithmetic follows precedence and a zero divisor reads 0", "[script]")
{
    CHECK(runFirstFunction("int value() { return 2 + 3 * 4 - -1; }") == 15);
    CHECK(runFirstFunction("int value() { return 7 / 0 + 7 % 0 + 1; }") == 1);
    CHECK(runFirstFunction("int value() { return -(3 - 5); }") == 2);
}

TEST_CASE("a decimal stored in an int truncates toward zero, as C does", "[script]")
{
    CHECK(runFirstFunction("int value() { int x = 7 * 0.5; return x; }") == 3);
    CHECK(runFirstFunction("int value() { int x = -2.7; return x; }") == -2);
    CHECK(runFirstFunction("int value() { int x = 10; x += 0.9; return x; }") == 10);
    CHECK(runFirstFunction("int value() { int x = 10; x -= 0.5; return x; }") == 9);
    CHECK(runFirstFunction("int value() { double d = 1.5; int x = -d * 3; return x; }") == -4);
}

TEST_CASE("arguments and return values truncate into an int", "[script]")
{
    CHECK(runFirstFunction("int value() { return half(7); }\n"
                           "int half(int n) { return n * 0.5; }") == 3);
    CHECK(runFirstFunction("int value() { return identity(2.9); }\n"
                           "int identity(int n) { return n; }") == 2);
}

TEST_CASE("an int divided by an int stays whole until a decimal joins in", "[script]")
{
    CHECK(runFirstFunction("int value() { double d = 7 / 2; int x = d * 10; return x; }") == 30);
    CHECK(runFirstFunction("int value() { int x = 7 / 2.0 * 10; return x; }") == 35);
}

TEST_CASE("a float holds float precision and a double holds double precision", "[script]")
{
    CHECK(runFirstFunction("int value() { float f = 0.1; double d = 0.1; return f == d; }") == 0);
    CHECK(runFirstFunction("int value() { double d = 0.1; return d == 0.1; }") == 1);
    CHECK(runFirstFunction("int value() { float f = 16777216; f += 1; int x = f; return x; }") == 16777216);
    CHECK(runFirstFunction("int value() { double d = 16777216; d += 1; int x = d; return x; }") == 16777217);
}

TEST_CASE("a decimal too large for an int stores as the largest int", "[script]")
{
    CHECK(runFirstFunction("int value() { int x = 100000.0 * 100000.0; return x; }") == 2147483647);
    CHECK(runFirstFunction("int value() { int x = -100000.0 * 100000.0; return x; }") == -2147483647 - 1);
}

TEST_CASE("a decimal written to a node property truncates", "[script]")
{
    const ScriptCompileResult compiled = compileTraversalScript(classWith(
        "void main() { current.count = 5 * 0.5; current.switchCount += 1.9; }"));

    REQUIRE(compiled.succeeded());

    ScriptFixture fixture;
    ScriptRun     run(fixture.context(compiled.script, ScriptWrites::Commit, nullptr));

    run.call(compiled.script.mainFunction, {});

    CHECK(fixture.nodeState.get(NodeStateSlot::Count, 1) == 2);
    CHECK(fixture.nodeState.get(NodeStateSlot::SwitchCount, 1) == 1);
}

TEST_CASE("a comment ends at a closing slash pair or at the end of the line", "[script]")
{
    CHECK(runFirstFunction("int value() { int a = 1; // closed // int b = 2;\n// return 9;\nreturn a + b; }") == 3);
}

TEST_CASE("members keep their value between calls and start at 0 or none", "[script]")
{
    const ScriptCompileResult compiled = compileTraversalScript(classWith(
        "int ticks;\n"
        "Node last;\n"
        "int tick() { ticks += 1; if last == none { last = current; } return ticks; }"));

    REQUIRE(compiled.succeeded());
    REQUIRE(compiled.script.memberDefaults == std::vector<double> { 0, -1 });

    ScriptFixture fixture;

    fixture.members[1] = -1;

    for (int expected = 1; expected <= 3; ++expected) {
        ScriptRun run(fixture.context(compiled.script, ScriptWrites::Commit, nullptr));
        CHECK(run.call(0, {}) == expected);
    }

    CHECK(fixture.members[1] == 1);
}

TEST_CASE("a trial run reads its own writes and leaves the traversal untouched", "[script]")
{
    const ScriptCompileResult compiled = compileTraversalScript(classWith(
        "int marks;\n"
        "Node advance(int numSteps) {\n"
        "    current.count += 5;\n"
        "    marks += 1;\n"
        "    if current.count == 5 { return current.lastChild; }\n"
        "    return none;\n"
        "}"));

    REQUIRE(compiled.succeeded());

    const int arguments[] = { 1 };

    ScriptFixture fixture;

    {
        ScriptRun trial(fixture.context(compiled.script, ScriptWrites::Trial, nullptr));
        CHECK(trial.call(compiled.script.advanceFunction, arguments) == -1);
    }

    CHECK(fixture.nodeState.get(NodeStateSlot::Count, 1) == 0);
    CHECK(fixture.members[0] == 0);

    {
        ScriptRun committed(fixture.context(compiled.script, ScriptWrites::Commit, nullptr));
        committed.call(compiled.script.advanceFunction, arguments);
    }

    CHECK(fixture.nodeState.get(NodeStateSlot::Count, 1) == 5);
    CHECK(fixture.members[0] == 1);
}

TEST_CASE("a variable declared in a block is gone after it", "[script]")
{
    const ScriptCompileResult compiled = compileTraversalScript(classWith(
        "int value() {\n"
        "    if 1 == 1 { int a = 5; }\n"
        "    return a;\n"
        "}"));

    REQUIRE(compiled.diagnostics.size() == 1);
    CHECK(compiled.diagnostics.front().line == 5);
}

TEST_CASE("malformed scripts are rejected", "[script]")
{
    for (const char* body : { "int value() { x = 1; return x; }",
                              "int value() { break; }",
                              "int value() { continue; }",
                              "int value() { for a in current { } return 0; }",
                              "int value() { int a = current; return a; }",
                              "int value() { Node n = 1; return 0; }",
                              "int value() { current.pitch = 3; return 0; }",
                              "int value() { return current.bogus; }",
                              "int value() { return current + 1; }",
                              "int value() { int a = 1; return a.count; }",
                              "int value() { return 1 $ 2; }",
                              "int value() { return 99999999999; }",
                              "int value() { return 5.5 % 2; }",
                              "int value() { Node n = 1.5; return 0; }",
                              "int value() { double d = current; return 0; }",
                              "int value() { return; }",
                              "void main() { return 1; }",
                              "void main() { playNote(current, 1, 2); }",
                              "int main() { return 0; }",
                              "Node advance() { return none; }",
                              "Node advance(int n) { return advance(1); }",
                              "Node advance(int n) { playNote(current, 1, 2, 3); return none; }",
                              "void playNote() { }",
                              "int value() { return 0; } int value() { return 1; }",
                              "int ticks = 3;",
                              "void ticks;" }) {
        CAPTURE(body);
        CHECK_FALSE(compileTraversalScript(classWith(body)).succeeded());
    }
}

TEST_CASE("a diagnostic marks the whole offending expression", "[script]")
{
    const ScriptCompileResult compiled = compileTraversalScript(classWith(
        "int value() {\n"
        "    return current.bogus;\n"
        "}"));

    REQUIRE(compiled.diagnostics.size() == 1);
    CHECK(compiled.diagnostics.front().line   == 4);
    CHECK(compiled.diagnostics.front().column == 12);
    CHECK(compiled.diagnostics.front().length == 13);
}

TEST_CASE("the compiler refuses functions that would overflow the traversal stack", "[script]")
{
    std::string manyLocals;
    std::string fittingLocals;
    std::string deepExpression = "1";

    for (int local = 0; local <= RTScript::maxLocals; ++local) {
        manyLocals += "int v" + std::to_string(local) + " = 1;\n";
    }

    for (int local = 0; local < RTScript::maxLocals; ++local) {
        fittingLocals += "int v" + std::to_string(local) + " = 1;\n";
    }

    for (int depth = 0; depth <= RTScript::maxStack; ++depth) {
        deepExpression = "1 + (" + deepExpression + ")";
    }

    CHECK_FALSE(compileTraversalScript(classWith("int value() {\n" + manyLocals + "return 1; }")).succeeded());
    CHECK_FALSE(compileTraversalScript(classWith("int value() { return " + deepExpression + "; }")).succeeded());
    CHECK(compileTraversalScript(classWith("int value() {\n" + fittingLocals + "return 1; }")).succeeded());
}

TEST_CASE("core::random spreads evenly within the given fraction of its value", "[script]")
{
    const ScriptCompileResult compiled = compileTraversalScript("import core;\n" + classWith(
        "int value() {\n"
        "    int outside = 0;\n"
        "    int low     = 0;\n"
        "    int high    = 0;\n"
        "    int draw    = 0;\n"
        "    while draw < 250 {\n"
        "        float sample = core::random(50, 0.5);\n"
        "        if sample < 25 or sample >= 75 { outside += 1; }\n"
        "        if sample < 31.25 { low += 1; }\n"
        "        if sample >= 68.75 { high += 1; }\n"
        "        draw += 1;\n"
        "    }\n"
        "    return outside * 1000000 + low * 1000 + high;\n"
        "}"));

    REQUIRE(compiled.succeeded());

    ScriptFixture fixture;
    ScriptRun     run(fixture.context(compiled.script, ScriptWrites::Commit, nullptr));

    const int tally = run.call(0, {});

    CHECK(tally / 1000000 == 0);
    CHECK(tally / 1000 % 1000 > 0);
    CHECK(tally % 1000 > 0);
}

TEST_CASE("core::random gives a new value per draw and the same values for the same step", "[script]")
{
    const ScriptCompileResult compiled = compileTraversalScript("import core;\n" + classWith(
        "int value() { return core::random(1000000, 0.5); }\n"
        "int differs() { double a = core::random(1000000, 0.5); double b = core::random(1000000, 0.5); "
        "return a != b; }"));

    REQUIRE(compiled.succeeded());

    ScriptFixture firstFixture;
    ScriptFixture replayFixture;
    ScriptRun     firstRun (firstFixture.context(compiled.script, ScriptWrites::Commit, nullptr));
    ScriptRun     replayRun(replayFixture.context(compiled.script, ScriptWrites::Commit, nullptr));
    ScriptRun     pairRun  (firstFixture.context(compiled.script, ScriptWrites::Commit, nullptr));

    CHECK(firstRun.call(0, {}) == replayRun.call(0, {}));
    CHECK(pairRun.call(1, {}) == 1);
}

TEST_CASE("core functions need their import and must exist", "[script]")
{
    const ScriptCompileResult missingImport = compileTraversalScript(classWith(
        "int value() { return core::random(50, 0.5); }"));

    REQUIRE(missingImport.diagnostics.size() == 1);
    CHECK(missingImport.diagnostics.front().message.find("import core;") != std::string::npos);

    CHECK_FALSE(compileTraversalScript("import maths;\n" + classWith("int value() { return 0; }")).succeeded());
    CHECK_FALSE(compileTraversalScript("import core\n" + classWith("int value() { return 0; }")).succeeded());
    CHECK_FALSE(compileTraversalScript("import core;\n" + classWith(
        "int value() { return core::nothing(1); }")).succeeded());
    CHECK_FALSE(compileTraversalScript("import core;\n" + classWith(
        "int value() { return core::random(50); }")).succeeded());
    CHECK_FALSE(compileTraversalScript("import core;\n" + classWith(
        "int value() { return core::random; }")).succeeded());
}
