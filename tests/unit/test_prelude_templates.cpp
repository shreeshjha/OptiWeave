#include "optiweave/templates/prelude.hpp"
#include <gtest/gtest.h>

TEST(PreludeTemplates, ArithmeticOps) {
  int a = 6, b = 3;
  // Arithmetic
  EXPECT_EQ(optiweave::__primop_add<int, int>()(a, b), 9);
  EXPECT_EQ(optiweave::__primop_sub<int, int>()(a, b), 3);
  EXPECT_EQ(optiweave::__primop_mul<int, int>()(a, b), 18);
  EXPECT_EQ(optiweave::__primop_div<int, int>()(a, b), 2);
  EXPECT_EQ(optiweave::__primop_rem<int, int>()(a, b), 0);
}

TEST(PreludeTemplates, AssignmentAndComparisons) {
  int x = 1, y = 2;
  // Assignment
  EXPECT_EQ(&optiweave::__primop_assign<int, int>()(x, y), &x);
  EXPECT_EQ(x, 2);
  // Compound assignments
  EXPECT_EQ(&optiweave::__primop_add_assign<int, int>()(x, 3), &x);
  EXPECT_EQ(x, 5);
  EXPECT_EQ(&optiweave::__primop_sub_assign<int, int>()(x, 1), &x);
  EXPECT_EQ(x, 4);
  EXPECT_EQ(&optiweave::__primop_mul_assign<int, int>()(x, 2), &x);
  EXPECT_EQ(x, 8);
  EXPECT_EQ(&optiweave::__primop_div_assign<int, int>()(x, 2), &x);
  EXPECT_EQ(x, 4);
  EXPECT_EQ(&optiweave::__primop_rem_assign<int, int>()(x, 3), &x);
  EXPECT_EQ(x, 1);
  // Comparisons
  EXPECT_TRUE(optiweave::__primop_eq<int, int>()(x, y));  // 2 == 2
  EXPECT_FALSE(optiweave::__primop_ne<int, int>()(x, y));
  EXPECT_FALSE(optiweave::__primop_lt<int, int>()(x, y));
  EXPECT_FALSE(optiweave::__primop_gt<int, int>()(x, y));
  EXPECT_TRUE(optiweave::__primop_le<int, int>()(x, y));
  EXPECT_TRUE(optiweave::__primop_ge<int, int>()(x, y));
}
