#include "optiweave/analysis/dependency_graph.hpp"
#include <gtest/gtest.h>

using namespace optiweave::analysis;

class DependencyGraphTest : public ::testing::Test {
protected:
  void SetUp() override {
    graph_ = DependencyGraph();
  }

  DependencyGraph graph_;
};

// Test DependencyNode structure initialization
TEST_F(DependencyGraphTest, DependencyNodeStructure) {
  DependencyNode node;

  EXPECT_TRUE(node.filepath.empty());
  EXPECT_TRUE(node.includes.empty());
  EXPECT_TRUE(node.included_by.empty());
  EXPECT_FALSE(node.is_system_header);
  EXPECT_EQ(node.include_depth, -1);
  EXPECT_EQ(node.fan_out, 0u);
  EXPECT_EQ(node.fan_in, 0u);
}

// Test adding files
TEST_F(DependencyGraphTest, AddFile) {
  graph_.add_file("main.cpp", false);
  graph_.add_file("foo.h", false);
  graph_.add_file("bar.h", false);
  graph_.add_file("iostream", true);  // System header

  std::vector<std::string> all_files = graph_.get_all_files();
  EXPECT_EQ(all_files.size(), 4u);
}

// Test getting node
TEST_F(DependencyGraphTest, GetNode) {
  graph_.add_file("test.cpp", false);

  const DependencyNode* node = graph_.get_node("test.cpp");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->filepath, "test.cpp");
  EXPECT_FALSE(node->is_system_header);

  const DependencyNode* missing = graph_.get_node("nonexistent.cpp");
  EXPECT_EQ(missing, nullptr);
}

// Test system header flag
TEST_F(DependencyGraphTest, SystemHeaderFlag) {
  graph_.add_file("user_header.h", false);
  graph_.add_file("vector", true);

  const DependencyNode* user = graph_.get_node("user_header.h");
  ASSERT_NE(user, nullptr);
  EXPECT_FALSE(user->is_system_header);

  const DependencyNode* system = graph_.get_node("vector");
  ASSERT_NE(system, nullptr);
  EXPECT_TRUE(system->is_system_header);
}

// Test adding include relationships
TEST_F(DependencyGraphTest, AddInclude) {
  graph_.add_file("main.cpp", false);
  graph_.add_file("foo.h", false);
  graph_.add_file("bar.h", false);

  graph_.add_include("main.cpp", "foo.h");
  graph_.add_include("main.cpp", "bar.h");
  graph_.add_include("foo.h", "bar.h");

  const DependencyNode* main_node = graph_.get_node("main.cpp");
  ASSERT_NE(main_node, nullptr);
  EXPECT_EQ(main_node->includes.size(), 2u);
  EXPECT_TRUE(main_node->includes.count("foo.h") > 0);
  EXPECT_TRUE(main_node->includes.count("bar.h") > 0);

  const DependencyNode* bar_node = graph_.get_node("bar.h");
  ASSERT_NE(bar_node, nullptr);
  EXPECT_EQ(bar_node->included_by.size(), 2u);
  EXPECT_TRUE(bar_node->included_by.count("main.cpp") > 0);
  EXPECT_TRUE(bar_node->included_by.count("foo.h") > 0);
}

// Test finding root files
TEST_F(DependencyGraphTest, GetRootFiles) {
  graph_.add_file("main.cpp", false);
  graph_.add_file("foo.h", false);
  graph_.add_file("bar.h", false);

  graph_.add_include("main.cpp", "foo.h");
  graph_.add_include("foo.h", "bar.h");

  std::vector<std::string> root_files = graph_.get_root_files(false);

  EXPECT_EQ(root_files.size(), 1u);
  EXPECT_EQ(root_files[0], "main.cpp");
}

// Test finding leaf files
TEST_F(DependencyGraphTest, GetLeafFiles) {
  graph_.add_file("main.cpp", false);
  graph_.add_file("foo.h", false);
  graph_.add_file("bar.h", false);

  graph_.add_include("main.cpp", "foo.h");
  graph_.add_include("main.cpp", "bar.h");

  std::vector<std::string> leaf_files = graph_.get_leaf_files(false);

  EXPECT_EQ(leaf_files.size(), 2u);
  // Check that both foo.h and bar.h are in the result
  bool has_foo = false;
  bool has_bar = false;
  for (const auto& file : leaf_files) {
    if (file == "foo.h") has_foo = true;
    if (file == "bar.h") has_bar = true;
  }
  EXPECT_TRUE(has_foo);
  EXPECT_TRUE(has_bar);
}

// Test circular dependency detection - simple cycle
TEST_F(DependencyGraphTest, DetectCircularDependency) {
  graph_.add_file("a.h", false);
  graph_.add_file("b.h", false);
  graph_.add_file("c.h", false);

  // Create cycle: a -> b -> c -> a
  graph_.add_include("a.h", "b.h");
  graph_.add_include("b.h", "c.h");
  graph_.add_include("c.h", "a.h");

  std::vector<std::vector<std::string>> cycles = graph_.detect_circular_dependencies();

  EXPECT_GT(cycles.size(), 0u);
}

// Test no circular dependencies
TEST_F(DependencyGraphTest, NoCircularDependencies) {
  graph_.add_file("main.cpp", false);
  graph_.add_file("foo.h", false);
  graph_.add_file("bar.h", false);

  graph_.add_include("main.cpp", "foo.h");
  graph_.add_include("main.cpp", "bar.h");

  std::vector<std::vector<std::string>> cycles = graph_.detect_circular_dependencies();

  EXPECT_EQ(cycles.size(), 0u);
}

// Test self-include detection
TEST_F(DependencyGraphTest, DetectSelfInclude) {
  graph_.add_file("recursive.h", false);
  graph_.add_include("recursive.h", "recursive.h");

  std::vector<std::vector<std::string>> cycles = graph_.detect_circular_dependencies();

  EXPECT_GT(cycles.size(), 0u);
}

// Test include depth calculation
TEST_F(DependencyGraphTest, CalculateIncludeDepths) {
  graph_.add_file("main.cpp", false);
  graph_.add_file("level1.h", false);
  graph_.add_file("level2.h", false);
  graph_.add_file("level3.h", false);

  graph_.add_include("main.cpp", "level1.h");
  graph_.add_include("level1.h", "level2.h");
  graph_.add_include("level2.h", "level3.h");

  graph_.calculate_include_depths();

  const DependencyNode* main_node = graph_.get_node("main.cpp");
  const DependencyNode* level1_node = graph_.get_node("level1.h");
  const DependencyNode* level2_node = graph_.get_node("level2.h");
  const DependencyNode* level3_node = graph_.get_node("level3.h");

  ASSERT_NE(main_node, nullptr);
  ASSERT_NE(level1_node, nullptr);
  ASSERT_NE(level2_node, nullptr);
  ASSERT_NE(level3_node, nullptr);

  EXPECT_EQ(main_node->include_depth, 0);
  EXPECT_EQ(level1_node->include_depth, 1);
  EXPECT_EQ(level2_node->include_depth, 2);
  EXPECT_EQ(level3_node->include_depth, 3);
}

// Test metrics calculation
TEST_F(DependencyGraphTest, CalculateMetrics) {
  graph_.add_file("hub.h", false);
  graph_.add_file("a.h", false);
  graph_.add_file("b.h", false);
  graph_.add_file("c.h", false);

  // hub.h includes a, b, c (fan-out = 3)
  graph_.add_include("hub.h", "a.h");
  graph_.add_include("hub.h", "b.h");
  graph_.add_include("hub.h", "c.h");

  // a, b, c all include hub (hub has fan-in = 3)
  graph_.add_file("d.h", false);
  graph_.add_include("a.h", "d.h");
  graph_.add_include("b.h", "d.h");
  graph_.add_include("c.h", "d.h");

  graph_.calculate_metrics();

  const DependencyNode* hub_node = graph_.get_node("hub.h");
  ASSERT_NE(hub_node, nullptr);
  EXPECT_EQ(hub_node->fan_out, 3u);

  const DependencyNode* d_node = graph_.get_node("d.h");
  ASSERT_NE(d_node, nullptr);
  EXPECT_EQ(d_node->fan_in, 3u);
}

// Test getting highest coupling
TEST_F(DependencyGraphTest, GetHighestCoupling) {
  graph_.add_file("highly_coupled.h", false);
  graph_.add_file("a.h", false);
  graph_.add_file("b.h", false);
  graph_.add_file("c.h", false);

  graph_.add_include("highly_coupled.h", "a.h");
  graph_.add_include("highly_coupled.h", "b.h");
  graph_.add_include("highly_coupled.h", "c.h");

  graph_.calculate_metrics();

  std::vector<std::pair<std::string, size_t>> highest_coupling = 
      graph_.get_highest_coupling(1, false);

  EXPECT_GT(highest_coupling.size(), 0u);
  if (!highest_coupling.empty()) {
    EXPECT_EQ(highest_coupling[0].first, "highly_coupled.h");
    EXPECT_EQ(highest_coupling[0].second, 3u);
  }
}

// Test filtering out system headers
TEST_F(DependencyGraphTest, FilterSystemHeaders) {
  graph_.add_file("main.cpp", false);
  graph_.add_file("user.h", false);
  graph_.add_file("iostream", true);
  graph_.add_file("vector", true);

  graph_.add_include("main.cpp", "user.h");
  graph_.add_include("main.cpp", "iostream");
  graph_.add_include("user.h", "vector");

  // Get root files without system headers
  std::vector<std::string> root_files = graph_.get_root_files(false);
  EXPECT_EQ(root_files.size(), 1u);
  EXPECT_EQ(root_files[0], "main.cpp");

  // Get root files with system headers - should still be same since system headers
  // are included by other files, not root files themselves
  std::vector<std::string> root_files_with_system = graph_.get_root_files(true);
  EXPECT_EQ(root_files_with_system.size(), 1u);
  
  // Verify leaf files differ with system header filtering
  std::vector<std::string> leaf_without_system = graph_.get_leaf_files(false);
  std::vector<std::string> leaf_with_system = graph_.get_leaf_files(true);
  EXPECT_GT(leaf_with_system.size(), leaf_without_system.size());
}

// Test getting all files
TEST_F(DependencyGraphTest, GetAllFiles) {
  graph_.add_file("a.cpp", false);
  graph_.add_file("b.h", false);
  graph_.add_file("c.h", false);

  std::vector<std::string> all_files = graph_.get_all_files();

  EXPECT_EQ(all_files.size(), 3u);
}
