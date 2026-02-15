#include "optiweave/analysis/call_graph.hpp"
#include <gtest/gtest.h>

using namespace optiweave::analysis;

class CallGraphTest : public ::testing::Test {
protected:
  void SetUp() override {
    graph_ = CallGraph();
  }

  CallGraph graph_;
};

// Test CallGraphNode structure initialization
TEST_F(CallGraphTest, CallGraphNodeStructure) {
  CallGraphNode node;

  EXPECT_TRUE(node.function_name.empty());
  EXPECT_TRUE(node.file.empty());
  EXPECT_EQ(node.line, 0);
  EXPECT_FALSE(node.is_template);
  EXPECT_FALSE(node.is_virtual);
  EXPECT_TRUE(node.callees.empty());
  EXPECT_TRUE(node.callers.empty());
  EXPECT_EQ(node.call_count, 0u);
  EXPECT_EQ(node.total_time_ns, 0u);
}

// Test CallGraphNode with constructor
TEST_F(CallGraphTest, CallGraphNodeWithConstructor) {
  CallGraphNode node("foo", "main.cpp", 42);

  EXPECT_EQ(node.function_name, "foo");
  EXPECT_EQ(node.file, "main.cpp");
  EXPECT_EQ(node.line, 42);
  EXPECT_FALSE(node.is_template);
  EXPECT_FALSE(node.is_virtual);
}

// Test CallGraph initialization
TEST_F(CallGraphTest, CallGraphInitialization) {
  EXPECT_EQ(graph_.size(), 0u);
  EXPECT_EQ(graph_.edge_count(), 0u);
}

// Test adding functions
TEST_F(CallGraphTest, AddFunction) {
  graph_.add_function("main", "main.cpp", 10);
  graph_.add_function("foo", "foo.cpp", 20);
  graph_.add_function("bar", "bar.cpp", 30);

  EXPECT_EQ(graph_.size(), 3u);
  EXPECT_TRUE(graph_.has_function("main"));
  EXPECT_TRUE(graph_.has_function("foo"));
  EXPECT_TRUE(graph_.has_function("bar"));
  EXPECT_FALSE(graph_.has_function("baz"));
}

// Test getting function
TEST_F(CallGraphTest, GetFunction) {
  graph_.add_function("test_func", "test.cpp", 15);

  const CallGraphNode* node = graph_.get_function("test_func");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->function_name, "test_func");
  EXPECT_EQ(node->file, "test.cpp");
  EXPECT_EQ(node->line, 15);

  const CallGraphNode* missing = graph_.get_function("nonexistent");
  EXPECT_EQ(missing, nullptr);
}

// Test adding function calls (edges)
TEST_F(CallGraphTest, AddCall) {
  graph_.add_function("main", "main.cpp", 10);
  graph_.add_function("foo", "foo.cpp", 20);
  graph_.add_function("bar", "bar.cpp", 30);

  graph_.add_call("main", "foo");
  graph_.add_call("main", "bar");
  graph_.add_call("foo", "bar");

  EXPECT_EQ(graph_.edge_count(), 3u);

  // Verify callees
  const CallGraphNode* main_node = graph_.get_function("main");
  ASSERT_NE(main_node, nullptr);
  EXPECT_EQ(main_node->callees.size(), 2u);
  EXPECT_TRUE(main_node->callees.count("foo") > 0);
  EXPECT_TRUE(main_node->callees.count("bar") > 0);

  // Verify callers
  const CallGraphNode* foo_node = graph_.get_function("foo");
  ASSERT_NE(foo_node, nullptr);
  EXPECT_EQ(foo_node->callers.size(), 1u);
  EXPECT_TRUE(foo_node->callers.count("main") > 0);
}

// Test finding entry points
TEST_F(CallGraphTest, FindEntryPoints) {
  graph_.add_function("main", "main.cpp", 10);
  graph_.add_function("foo", "foo.cpp", 20);
  graph_.add_function("bar", "bar.cpp", 30);

  graph_.add_call("main", "foo");
  graph_.add_call("foo", "bar");

  std::vector<std::string> entry_points = graph_.find_entry_points();

  EXPECT_EQ(entry_points.size(), 1u);
  EXPECT_EQ(entry_points[0], "main");
}

// Test finding leaf functions
TEST_F(CallGraphTest, FindLeafFunctions) {
  graph_.add_function("main", "main.cpp", 10);
  graph_.add_function("foo", "foo.cpp", 20);
  graph_.add_function("bar", "bar.cpp", 30);

  graph_.add_call("main", "foo");
  graph_.add_call("main", "bar");

  std::vector<std::string> leaf_functions = graph_.find_leaf_functions();

  EXPECT_EQ(leaf_functions.size(), 2u);
  // Check that both foo and bar are in the result
  bool has_foo = false;
  bool has_bar = false;
  for (const auto& func : leaf_functions) {
    if (func == "foo") has_foo = true;
    if (func == "bar") has_bar = true;
  }
  EXPECT_TRUE(has_foo);
  EXPECT_TRUE(has_bar);
}

// Test cycle detection - simple cycle
TEST_F(CallGraphTest, DetectSimpleCycle) {
  graph_.add_function("a", "a.cpp", 10);
  graph_.add_function("b", "b.cpp", 20);
  graph_.add_function("c", "c.cpp", 30);

  // Create cycle: a -> b -> c -> a
  graph_.add_call("a", "b");
  graph_.add_call("b", "c");
  graph_.add_call("c", "a");

  std::vector<std::vector<std::string>> cycles = graph_.detect_cycles();

  EXPECT_GT(cycles.size(), 0u);
}

// Test no cycles
TEST_F(CallGraphTest, NoCycles) {
  graph_.add_function("main", "main.cpp", 10);
  graph_.add_function("foo", "foo.cpp", 20);
  graph_.add_function("bar", "bar.cpp", 30);

  graph_.add_call("main", "foo");
  graph_.add_call("main", "bar");

  std::vector<std::vector<std::string>> cycles = graph_.detect_cycles();

  EXPECT_EQ(cycles.size(), 0u);
}

// Test self-recursion detection
TEST_F(CallGraphTest, DetectSelfRecursion) {
  graph_.add_function("recursive", "recursive.cpp", 10);
  graph_.add_call("recursive", "recursive");

  std::vector<std::vector<std::string>> cycles = graph_.detect_cycles();

  EXPECT_GT(cycles.size(), 0u);
}

// Test call depth calculation
TEST_F(CallGraphTest, CalculateCallDepths) {
  graph_.add_function("main", "main.cpp", 10);
  graph_.add_function("level1", "level1.cpp", 20);
  graph_.add_function("level2", "level2.cpp", 30);
  graph_.add_function("level3", "level3.cpp", 40);

  graph_.add_call("main", "level1");
  graph_.add_call("level1", "level2");
  graph_.add_call("level2", "level3");

  std::unordered_map<std::string, int> depths = graph_.calculate_call_depths();

  // Note: The implementation returns depth from leaf nodes (inverted)
  // main is furthest from leaves (depth 3), level3 is a leaf (depth 0)
  EXPECT_EQ(depths["main"], 3);
  EXPECT_EQ(depths["level1"], 2);
  EXPECT_EQ(depths["level2"], 1);
  EXPECT_EQ(depths["level3"], 0);
}

// Test runtime statistics update
TEST_F(CallGraphTest, UpdateRuntimeStats) {
  graph_.add_function("hot_function", "hot.cpp", 100);

  graph_.update_runtime_stats("hot_function", 1000, 5000000);

  const CallGraphNode* node = graph_.get_function("hot_function");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->call_count, 1000u);
  EXPECT_EQ(node->total_time_ns, 5000000u);
}

// Test getting all functions
TEST_F(CallGraphTest, GetAllFunctions) {
  graph_.add_function("a", "a.cpp", 10);
  graph_.add_function("b", "b.cpp", 20);
  graph_.add_function("c", "c.cpp", 30);

  std::vector<std::string> all_functions = graph_.get_all_functions();

  EXPECT_EQ(all_functions.size(), 3u);
}

// Test template and virtual function flags
TEST_F(CallGraphTest, TemplateAndVirtualFlags) {
  graph_.add_function("template_func", "template.cpp", 10, true, false);
  graph_.add_function("virtual_func", "virtual.cpp", 20, false, true);
  graph_.add_function("both_func", "both.cpp", 30, true, true);

  const CallGraphNode* template_node = graph_.get_function("template_func");
  ASSERT_NE(template_node, nullptr);
  EXPECT_TRUE(template_node->is_template);
  EXPECT_FALSE(template_node->is_virtual);

  const CallGraphNode* virtual_node = graph_.get_function("virtual_func");
  ASSERT_NE(virtual_node, nullptr);
  EXPECT_FALSE(virtual_node->is_template);
  EXPECT_TRUE(virtual_node->is_virtual);

  const CallGraphNode* both_node = graph_.get_function("both_func");
  ASSERT_NE(both_node, nullptr);
  EXPECT_TRUE(both_node->is_template);
  EXPECT_TRUE(both_node->is_virtual);
}

// Test CallGraphBuilder
TEST_F(CallGraphTest, CallGraphBuilder) {
  CallGraphBuilder builder;

  builder.enter_function("main", "main.cpp", 10);
  builder.record_call("foo");
  builder.record_call("bar");
  builder.exit_function();

  builder.enter_function("foo", "foo.cpp", 20);
  builder.record_call("baz");
  builder.exit_function();

  const CallGraph& built_graph = builder.get_graph();

  EXPECT_TRUE(built_graph.has_function("main"));
  EXPECT_TRUE(built_graph.has_function("foo"));
  EXPECT_GT(built_graph.edge_count(), 0u);
}
