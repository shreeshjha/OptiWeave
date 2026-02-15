#include "optiweave/analysis/data_flow_analysis.hpp"
#include <gtest/gtest.h>

using namespace optiweave::analysis;

class DataFlowAnalysisTest : public ::testing::Test {
protected:
  void SetUp() override {
    analysis_ = DataFlowAnalysis();
  }

  DataFlowAnalysis analysis_;
};

// Test VariableState enum values
TEST_F(DataFlowAnalysisTest, VariableStateEnumValues) {
  VariableState declared = VariableState::Declared;
  VariableState initialized = VariableState::Initialized;
  VariableState used = VariableState::Used;
  VariableState unused = VariableState::Unused;
  VariableState maybe_uninit = VariableState::MaybeUninitialized;

  EXPECT_EQ(declared, VariableState::Declared);
  EXPECT_EQ(initialized, VariableState::Initialized);
  EXPECT_EQ(used, VariableState::Used);
  EXPECT_EQ(unused, VariableState::Unused);
  EXPECT_EQ(maybe_uninit, VariableState::MaybeUninitialized);
  EXPECT_NE(declared, initialized);
}

// Test VariableInfo structure initialization
TEST_F(DataFlowAnalysisTest, VariableInfoStructure) {
  VariableInfo info;

  EXPECT_TRUE(info.name.empty());
  EXPECT_TRUE(info.type.empty());
  EXPECT_TRUE(info.function_context.empty());
  EXPECT_FALSE(info.has_initializer);
  EXPECT_FALSE(info.is_parameter);
  EXPECT_FALSE(info.is_global);
  EXPECT_EQ(info.state, VariableState::Declared);
  EXPECT_TRUE(info.uses.empty());
  EXPECT_TRUE(info.definitions.empty());
}

// Test VariableInfo with constructor
TEST_F(DataFlowAnalysisTest, VariableInfoWithConstructor) {
  clang::SourceLocation dummy_loc;
  VariableInfo info("x", "int", dummy_loc, "foo");

  EXPECT_EQ(info.name, "x");
  EXPECT_EQ(info.type, "int");
  EXPECT_EQ(info.function_context, "foo");
  EXPECT_EQ(info.state, VariableState::Declared);
}

// Test VariableInfo is_used and is_defined
TEST_F(DataFlowAnalysisTest, VariableInfoUsedAndDefined) {
  clang::SourceLocation dummy_loc;
  VariableInfo info;
  info.has_initializer = false;

  EXPECT_FALSE(info.is_used());
  EXPECT_FALSE(info.is_defined());

  info.add_use(dummy_loc, "foo", true);
  EXPECT_TRUE(info.is_used());

  info.has_initializer = true;
  EXPECT_TRUE(info.is_defined());
}

// Test VariableUse structure
TEST_F(DataFlowAnalysisTest, VariableUseStructure) {
  clang::SourceLocation dummy_loc;
  VariableUse use(dummy_loc, "bar", true);

  EXPECT_EQ(use.context, "bar");
  EXPECT_TRUE(use.is_read);

  VariableUse write(dummy_loc, "baz", false);
  EXPECT_EQ(write.context, "baz");
  EXPECT_FALSE(write.is_read);
}

// Test DeadCodeInfo structure
TEST_F(DataFlowAnalysisTest, DeadCodeInfoStructure) {
  clang::SourceLocation dummy_loc;
  DeadCodeInfo info(dummy_loc, "unreachable code", "test_func");

  EXPECT_EQ(info.description, "unreachable code");
  EXPECT_EQ(info.function_context, "test_func");
}

// Test RefactoringOpportunity enum types
TEST_F(DataFlowAnalysisTest, RefactoringOpportunityTypes) {
  using Type = RefactoringOpportunity::Type;

  Type unused = Type::UnusedVariable;
  Type uninit = Type::UninitializedVariable;
  Type dead = Type::DeadCode;
  Type never_read = Type::VariableNeverRead;
  Type never_written = Type::VariableNeverWritten;

  EXPECT_EQ(unused, Type::UnusedVariable);
  EXPECT_EQ(uninit, Type::UninitializedVariable);
  EXPECT_EQ(dead, Type::DeadCode);
  EXPECT_EQ(never_read, Type::VariableNeverRead);
  EXPECT_EQ(never_written, Type::VariableNeverWritten);
  EXPECT_NE(unused, uninit);
}

// Test RefactoringOpportunity structure
TEST_F(DataFlowAnalysisTest, RefactoringOpportunityStructure) {
  clang::SourceLocation dummy_loc;
  RefactoringOpportunity opp(
      RefactoringOpportunity::Type::UnusedVariable,
      dummy_loc,
      "Variable 'x' is declared but never used",
      "Remove unused variable or use it",
      "main"
  );

  EXPECT_EQ(opp.type, RefactoringOpportunity::Type::UnusedVariable);
  EXPECT_EQ(opp.description, "Variable 'x' is declared but never used");
  EXPECT_EQ(opp.suggestion, "Remove unused variable or use it");
  EXPECT_EQ(opp.function_context, "main");
}

// Test getting variables
TEST_F(DataFlowAnalysisTest, GetVariables) {
  const std::map<std::string, VariableInfo>& vars = analysis_.get_variables();
  EXPECT_TRUE(vars.empty());
}

// Test getting opportunities
TEST_F(DataFlowAnalysisTest, GetOpportunities) {
  const std::vector<RefactoringOpportunity>& opps = analysis_.get_opportunities();
  EXPECT_TRUE(opps.empty());
}

// Test getting dead code
TEST_F(DataFlowAnalysisTest, GetDeadCode) {
  const std::vector<DeadCodeInfo>& dead = analysis_.get_dead_code();
  EXPECT_TRUE(dead.empty());
}

// Test multiple variable states
TEST_F(DataFlowAnalysisTest, MultipleVariableStates) {
  std::vector<VariableInfo> vars;

  clang::SourceLocation dummy_loc;

  VariableInfo var1("a", "int", dummy_loc, "foo");
  var1.state = VariableState::Declared;
  vars.push_back(var1);

  VariableInfo var2("b", "double", dummy_loc, "foo");
  var2.state = VariableState::Initialized;
  var2.has_initializer = true;
  vars.push_back(var2);

  VariableInfo var3("c", "float", dummy_loc, "bar");
  var3.state = VariableState::Used;
  var3.has_initializer = true;
  var3.add_use(dummy_loc, "bar", true);
  vars.push_back(var3);

  EXPECT_EQ(vars.size(), 3u);
  EXPECT_EQ(vars[0].state, VariableState::Declared);
  EXPECT_EQ(vars[1].state, VariableState::Initialized);
  EXPECT_EQ(vars[2].state, VariableState::Used);
  EXPECT_TRUE(vars[2].is_used());
}

// Test parameter vs local variable
TEST_F(DataFlowAnalysisTest, ParameterVsLocal) {
  clang::SourceLocation dummy_loc;

  VariableInfo param("arg", "int", dummy_loc, "func");
  param.is_parameter = true;

  VariableInfo local("temp", "int", dummy_loc, "func");
  local.is_parameter = false;

  EXPECT_TRUE(param.is_parameter);
  EXPECT_FALSE(local.is_parameter);
}

// Test global vs local variable
TEST_F(DataFlowAnalysisTest, GlobalVsLocal) {
  clang::SourceLocation dummy_loc;

  VariableInfo global("g_var", "int", dummy_loc, "");
  global.is_global = true;

  VariableInfo local("l_var", "int", dummy_loc, "func");
  local.is_global = false;

  EXPECT_TRUE(global.is_global);
  EXPECT_FALSE(local.is_global);
}

// Test read vs write tracking
TEST_F(DataFlowAnalysisTest, ReadVsWrite) {
  clang::SourceLocation dummy_loc;
  VariableInfo info;

  info.add_use(dummy_loc, "func", true);  // Read
  info.add_use(dummy_loc, "func", false); // Write

  EXPECT_EQ(info.uses.size(), 2u);
  EXPECT_TRUE(info.uses[0].is_read);
  EXPECT_FALSE(info.uses[1].is_read);
}

// Test multiple refactoring opportunities
TEST_F(DataFlowAnalysisTest, MultipleRefactoringOpportunities) {
  std::vector<RefactoringOpportunity> opps;

  clang::SourceLocation dummy_loc;

  opps.push_back(RefactoringOpportunity(
      RefactoringOpportunity::Type::UnusedVariable,
      dummy_loc,
      "Variable unused",
      "Remove it",
      "main"
  ));

  opps.push_back(RefactoringOpportunity(
      RefactoringOpportunity::Type::UninitializedVariable,
      dummy_loc,
      "Variable uninitialized",
      "Initialize it",
      "main"
  ));

  opps.push_back(RefactoringOpportunity(
      RefactoringOpportunity::Type::DeadCode,
      dummy_loc,
      "Dead code detected",
      "Remove it",
      "helper"
  ));

  EXPECT_EQ(opps.size(), 3u);
  EXPECT_EQ(opps[0].type, RefactoringOpportunity::Type::UnusedVariable);
  EXPECT_EQ(opps[1].type, RefactoringOpportunity::Type::UninitializedVariable);
  EXPECT_EQ(opps[2].type, RefactoringOpportunity::Type::DeadCode);
}

// Test filtering opportunities by type
TEST_F(DataFlowAnalysisTest, FilterOpportunitiesByType) {
  std::vector<RefactoringOpportunity> all_opps;
  clang::SourceLocation dummy_loc;

  all_opps.push_back(RefactoringOpportunity(
      RefactoringOpportunity::Type::UnusedVariable,
      dummy_loc, "desc1", "sugg1", "func1"
  ));

  all_opps.push_back(RefactoringOpportunity(
      RefactoringOpportunity::Type::DeadCode,
      dummy_loc, "desc2", "sugg2", "func2"
  ));

  all_opps.push_back(RefactoringOpportunity(
      RefactoringOpportunity::Type::UnusedVariable,
      dummy_loc, "desc3", "sugg3", "func3"
  ));

  std::vector<RefactoringOpportunity> unused_vars;
  for (const auto& opp : all_opps) {
    if (opp.type == RefactoringOpportunity::Type::UnusedVariable) {
      unused_vars.push_back(opp);
    }
  }

  EXPECT_EQ(unused_vars.size(), 2u);
  EXPECT_EQ(unused_vars[0].description, "desc1");
  EXPECT_EQ(unused_vars[1].description, "desc3");
}
