#include <gtest/gtest.h>
#include <optiweave/analysis/pattern_detector.hpp>
#include <optiweave/analysis/pattern_names.hpp>
#include <optiweave/core/rule_config.hpp>

using namespace optiweave::analysis;
using namespace optiweave::core;

// Minimal stub for HotspotTracker (we only need it to compile)
// The detectors primarily use LoopInfo, not hotspot data.

class PatternDetectorTest : public ::testing::Test {
protected:
    RuleConfig config;
    optiweave::hotspots::HotspotTracker tracker;
    optiweave::statistics::OperationCounters stats;

    void SetUp() override {
        config = RuleConfig{};
    }

    LoopInfo makeHotLoop(int nesting = 1, bool strided = false,
                          int stride = 1, uint64_t time_ns = 100000,
                          uint64_t iters = 10000) {
        LoopInfo loop;
        loop.location.file = "test.c";
        loop.location.line = 42;
        loop.location.function = "test_func";
        loop.line_start = 42;
        loop.line_end = 50;
        loop.nesting_level = nesting;
        loop.has_divisions = false;
        loop.has_constant_divisor = false;
        loop.has_strided_access = strided;
        loop.stride_value = stride;
        loop.is_vectorizable = false;
        loop.total_time_ns = time_ns;
        loop.iteration_count = iters;
        loop.num_pointer_params = 0;
        return loop;
    }
};

// --- LoopInterchangeDetector ---

TEST_F(PatternDetectorTest, LoopInterchange_TriggersOnNestedStridedHotLoop) {
    LoopInterchangeDetector detector;
    auto loop = makeHotLoop(2, true, 16); // nesting=2, strided, stride=16
    // stride * element_size = 16 * 8 = 128 > 64 (cache line)
    std::vector<LoopInfo> loops = {loop};

    detector.analyze(tracker, stats, loops, config);
    auto patterns = detector.get_patterns();
    ASSERT_EQ(patterns.size(), 1u);
    EXPECT_EQ(patterns[0].pattern_name, std::string(patterns::kLoopInterchange));
}

TEST_F(PatternDetectorTest, LoopInterchange_SkipsSingleLevel) {
    LoopInterchangeDetector detector;
    auto loop = makeHotLoop(1, true, 16); // nesting=1
    std::vector<LoopInfo> loops = {loop};

    detector.analyze(tracker, stats, loops, config);
    EXPECT_EQ(detector.get_patterns().size(), 0u);
}

TEST_F(PatternDetectorTest, LoopInterchange_SkipsSmallStride) {
    LoopInterchangeDetector detector;
    auto loop = makeHotLoop(2, true, 4); // stride=4, 4*8=32 <= 64
    std::vector<LoopInfo> loops = {loop};

    detector.analyze(tracker, stats, loops, config);
    EXPECT_EQ(detector.get_patterns().size(), 0u);
}

// --- StrengthReductionDetector ---

TEST_F(PatternDetectorTest, StrengthReduction_TriggersOnMultiplication) {
    StrengthReductionDetector detector;
    auto loop = makeHotLoop();
    loop.operations_in_loop.push_back("multiplication");
    std::vector<LoopInfo> loops = {loop};

    detector.analyze(tracker, stats, loops, config);
    auto patterns = detector.get_patterns();
    ASSERT_EQ(patterns.size(), 1u);
    EXPECT_EQ(patterns[0].pattern_name, std::string(patterns::kStrengthReduction));
}

TEST_F(PatternDetectorTest, StrengthReduction_SkipsWithoutMultiplication) {
    StrengthReductionDetector detector;
    auto loop = makeHotLoop();
    loop.operations_in_loop.push_back("addition");
    std::vector<LoopInfo> loops = {loop};

    detector.analyze(tracker, stats, loops, config);
    EXPECT_EQ(detector.get_patterns().size(), 0u);
}

TEST_F(PatternDetectorTest, StrengthReduction_SkipsColdLoop) {
    StrengthReductionDetector detector;
    auto loop = makeHotLoop(1, false, 1, 100); // 100 ns — cold
    loop.operations_in_loop.push_back("multiplication");
    std::vector<LoopInfo> loops = {loop};

    detector.analyze(tracker, stats, loops, config);
    EXPECT_EQ(detector.get_patterns().size(), 0u);
}

// --- LoopUnrollHintDetector ---

TEST_F(PatternDetectorTest, LoopUnrollHint_TriggersOnSmallBoundedLoop) {
    LoopUnrollHintDetector detector;
    auto loop = makeHotLoop();
    loop.line_start = 42;
    loop.line_end = 46; // 4 stmts
    loop.iteration_count = 32; // <= max_trip_count (64)
    std::vector<LoopInfo> loops = {loop};

    detector.analyze(tracker, stats, loops, config);
    auto patterns = detector.get_patterns();
    ASSERT_EQ(patterns.size(), 1u);
    EXPECT_EQ(patterns[0].pattern_name, std::string(patterns::kLoopUnrollHint));
}

TEST_F(PatternDetectorTest, LoopUnrollHint_SkipsLargeBody) {
    LoopUnrollHintDetector detector;
    auto loop = makeHotLoop();
    loop.line_start = 42;
    loop.line_end = 60; // 18 stmts > max_body_stmts (8)
    loop.iteration_count = 32;
    std::vector<LoopInfo> loops = {loop};

    detector.analyze(tracker, stats, loops, config);
    EXPECT_EQ(detector.get_patterns().size(), 0u);
}

TEST_F(PatternDetectorTest, LoopUnrollHint_SkipsLargeTripCount) {
    LoopUnrollHintDetector detector;
    auto loop = makeHotLoop();
    loop.line_start = 42;
    loop.line_end = 46;
    loop.iteration_count = 1000; // > max_trip_count (64)
    std::vector<LoopInfo> loops = {loop};

    detector.analyze(tracker, stats, loops, config);
    EXPECT_EQ(detector.get_patterns().size(), 0u);
}

// --- PrefetchHintDetector ---

TEST_F(PatternDetectorTest, PrefetchHint_TriggersOnStridedSingleLevel) {
    PrefetchHintDetector detector;
    auto loop = makeHotLoop(1, true, 16); // stride=16, 16*8=128 >= 64 (min_stride)
    std::vector<LoopInfo> loops = {loop};

    detector.analyze(tracker, stats, loops, config);
    auto patterns = detector.get_patterns();
    ASSERT_EQ(patterns.size(), 1u);
    EXPECT_EQ(patterns[0].pattern_name, std::string(patterns::kPrefetchHint));
}

TEST_F(PatternDetectorTest, PrefetchHint_SkipsNestedLoops) {
    PrefetchHintDetector detector;
    auto loop = makeHotLoop(2, true, 16); // nesting=2
    std::vector<LoopInfo> loops = {loop};

    detector.analyze(tracker, stats, loops, config);
    EXPECT_EQ(detector.get_patterns().size(), 0u);
}

TEST_F(PatternDetectorTest, PrefetchHint_SkipsSmallStride) {
    PrefetchHintDetector detector;
    auto loop = makeHotLoop(1, true, 2); // stride=2, 2*8=16 < 64
    std::vector<LoopInfo> loops = {loop};

    detector.analyze(tracker, stats, loops, config);
    EXPECT_EQ(detector.get_patterns().size(), 0u);
}

// --- RestrictQualifierDetector ---

TEST_F(PatternDetectorTest, RestrictQualifier_TriggersOnTwoPlusPointerParams) {
    RestrictQualifierDetector detector;
    auto loop = makeHotLoop();
    loop.num_pointer_params = 3;
    std::vector<LoopInfo> loops = {loop};

    detector.analyze(tracker, stats, loops, config);
    auto patterns = detector.get_patterns();
    ASSERT_EQ(patterns.size(), 1u);
    EXPECT_EQ(patterns[0].pattern_name, std::string(patterns::kRestrictQualifier));
}

TEST_F(PatternDetectorTest, RestrictQualifier_SkipsFewerThanTwoPointerParams) {
    RestrictQualifierDetector detector;
    auto loop = makeHotLoop();
    loop.num_pointer_params = 1;
    std::vector<LoopInfo> loops = {loop};

    detector.analyze(tracker, stats, loops, config);
    EXPECT_EQ(detector.get_patterns().size(), 0u);
}

TEST_F(PatternDetectorTest, RestrictQualifier_SkipsColdLoop) {
    RestrictQualifierDetector detector;
    auto loop = makeHotLoop(1, false, 1, 100); // cold
    loop.num_pointer_params = 3;
    std::vector<LoopInfo> loops = {loop};

    detector.analyze(tracker, stats, loops, config);
    EXPECT_EQ(detector.get_patterns().size(), 0u);
}
