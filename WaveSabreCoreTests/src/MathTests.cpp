#include <gtest/gtest.h>

#include <format>

#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7Oscillator3Shape.hpp>

using namespace WaveSabreCore;

static constexpr float kEps = 1e-6f;

TEST(MathTests, MillisecondsToSamples)
{
  Helpers::SetSampleRate(44100);

  float samples = M7::math::MillisecondsToSamples(10.0f);
  // Allow small float error from 1/1000 factor in single-precision
  EXPECT_NEAR(samples, 441.0f, 1e-3f);
}

TEST(MathTests, FloatEqualsBasic)
{
  using M7::math::FloatEquals;
  EXPECT_TRUE(FloatEquals(1.0f, 1.0f + 5e-8f, 1e-6f));
  EXPECT_FALSE(FloatEquals(1.0f, 1.001f, 1e-4f));
}

TEST(ShapeTests, SawShapeEval)
{
  M7::WVShape saw = {.mSegments = {
                         M7::WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = 1, .beginAmp = -1.0f, .slope = +2.0f},
                     }};

  // test that evaluating around edge boundaries works as expected.
  auto testEval = [&](double phase01, float expectedAmp, float expectedSlope)
  {
    auto [amp, slope] = saw.EvalAmpSlopeAt(phase01);
    EXPECT_NEAR(amp, expectedAmp, kEps) << " at phase " << phase01;
    EXPECT_NEAR(slope, expectedSlope, kEps) << " at phase " << phase01;
  };

  testEval(0.0, -1.0f, +2.0f);
  testEval(0.5, 0.0f, +2.0f);

  // test very near 0
  testEval(1e-10, -1.0f + 2e-10f, +2.0f);
  testEval(1.0 - 1e-10, +1.0f - 2e-10f, +2.0f);
}

TEST(ShapeTests, TriangleShapeEval)
{
  M7::WVShape triangle = {
      .mSegments = {
          M7::WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = 0.5, .beginAmp = -1.0f, .slope = +4.0f},
          M7::WVSegment{.beginPhase01 = 0.5, .endPhaseIncluding1 = 1.0, .beginAmp = +1.0f, .slope = -4.0f},
      }};

  // test that evaluating around edge boundaries works as expected.
  auto testEval = [&](double phase01, float expectedAmp, float expectedSlope)
  {
    auto [amp, slope] = triangle.EvalAmpSlopeAt(phase01);
    auto msg = std::format("testing @ {:.3f}. expect=[{:.3f}, {:.3f}]; actual=[{:.3f}, {:.3f}]", phase01, expectedAmp, expectedSlope, amp, slope);
    std::cout << msg << std::endl;
    ::OutputDebugStringA((msg + "\n").c_str());
    EXPECT_NEAR(amp, expectedAmp, kEps) << " at phase " << phase01;
    EXPECT_NEAR(slope, expectedSlope, kEps) << " at phase " << phase01;
  };

  testEval(0.0, -1.0f, +4.0f);
  testEval(0.25, 0.0f, +4.0f);
  testEval(0.5, +1.0f, -4.0f);
  testEval(0.75, 0.0f, -4.0f);
  testEval(1.0, -1.0f, +4.0f);

  // test very near 0
  testEval(2e-10, -1.0f + 4e-10f, +4.0f);        // just after 0
  testEval(1.0 - 2e-10, -1.0f + 4e-10f, -4.0f);  // just before 1

  // test around midpoint
  testEval(0.5 - 2e-10, +1.0f - 4e-10f, +4.0f);
  testEval(0.5 + 2e-10, +1.0f - 4e-10f, -4.0f);
}

struct EdgeEncounter
{
  size_t segmentIndex;
  M7::WVSegment edge;
  float preAmp;
  float preSlope;
  float postAmp;
  float postSlope;
  double distFromPathStartToEdgeInPhase01;
  std::string reason;

  void TestPP(float expectedPreAmp, float expectedPreSlope, float expectedPostAmp, float expectedPostSlope)
  {
    EXPECT_NEAR(preAmp, expectedPreAmp, kEps);
    EXPECT_NEAR(preSlope, expectedPreSlope, kEps);
    EXPECT_NEAR(postAmp, expectedPostAmp, kEps);
    EXPECT_NEAR(postSlope, expectedPostSlope, kEps);
  }
};

TEST(ShapeTests, OneSegmentShapeWalking)
{
  //M7::WVShape saw = {.mSegments = {
  //                       M7::WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = 1, .beginAmp = -1.0f, .slope = +2.0f},
  //                   }};

  //{
  //  auto walker = saw.MakeWalker(0);
  //  std::vector<EdgeEncounter> encounters;

  //  auto visitEdge = [&](size_t segmentIndex,
  //                       const M7::WVSegment& edge,
  //                       float preA,
  //                       float preS,
  //                       float postA,
  //                       float postS,
  //                       double distFromPathStartToEdgeInPhase01
  //      //,std::string s
  //      )
  //  {
  //    cc::log(
  //        std::format("@ {:.3f}, enter segment #{}, left edge @ {:.3f} phase; pre[{:.3f}, {:.3f}] -> [{:.3f}, {:.3f}]",
  //                    walker.mCursorPhase01,
  //                    segmentIndex,
  //                    edge.beginPhase01,
  //                    preA,
  //                    preS,
  //                    postA,
  //                    postS));
  //    encounters.emplace_back(EdgeEncounter{
  //        .segmentIndex = segmentIndex,
  //        .edge = edge,
  //        .preAmp = preA,
  //        .preSlope = preS,
  //        .postAmp = postA,
  //        .postSlope = postS,
  //        .distFromPathStartToEdgeInPhase01 = distFromPathStartToEdgeInPhase01,
  //        //.reason = s,
  //    });
  //  };

  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));
  //  walker.Step(0.5, visitEdge);
  //  EXPECT_EQ(encounters.size(), 1);
  //  encounters.back().TestPP(0, 0, -1, 2);

  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));
  //  walker.Step(0.5, visitEdge);
  //  EXPECT_EQ(encounters.size(), 1);  // landing directly on next edge will eval after.

  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));
  //  walker.Step(0.5, visitEdge);
  //  EXPECT_EQ(encounters.size(), 2);
  //  encounters.back().TestPP(1, 2, -1, 2);

  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));
  //  walker.Step(0.5, visitEdge);
  //  EXPECT_EQ(encounters.size(), 2);

  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));
  //  walker.Step(0.5, visitEdge);
  //  EXPECT_EQ(encounters.size(), 3);

  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));
  //  walker.Step(0.5, visitEdge);
  //  EXPECT_EQ(encounters.size(), 3);
  //}
}

TEST(ShapeTests, OneSegmentShapeWalkingFuzzyUndershoot)
{
  //// test that undershooting an edge still triggers the edge.
  //M7::WVShape saw = {.mSegments = {
  //                       M7::WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = 1, .beginAmp = -1.0f, .slope = +2.0f},
  //                   }};

  //{
  //  auto walker = saw.MakeWalker(0);
  //  int encounters = 0;

  //  auto visitEdge = [&](size_t segmentIndex,
  //                       const M7::WVSegment& edge,
  //                       float preA,
  //                       float preS,
  //                       float postA,
  //                       float postS,
  //                       double distFromPathStartToEdgeInPhase01
  //      //,std::string s
  //      )
  //  {
  //    cc::log(
  //        std::format("@ {:.3f}, enter segment #{}, left edge @ {:.3f} phase; pre[{:.3f}, {:.3f}] -> [{:.3f}, {:.3f}]",
  //                    walker.mCursorPhase01,
  //                    segmentIndex,
  //                    edge.beginPhase01,
  //                    preA,
  //                    preS,
  //                    postA,
  //                    postS));
  //    encounters++;
  //  };

  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));
  //  walker.Step(0.50000 - 1e-5, visitEdge);
  //  EXPECT_EQ(encounters, 1);

  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));  // from 0.5 - almost 1
  //  walker.Step(0.5, visitEdge);
  //  EXPECT_EQ(encounters, 1);

  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));  // from almost 1 to almost 0.5
  //  walker.Step(0.5, visitEdge);
  //  EXPECT_EQ(encounters, 2);
  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));
  //  walker.Step(0.5, visitEdge);
  //  EXPECT_EQ(encounters, 2);
  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));
  //  walker.Step(0.5, visitEdge);
  //  EXPECT_EQ(encounters, 3);
  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));
  //  walker.Step(0.5, visitEdge);
  //  EXPECT_EQ(encounters, 3);
  //}
}

TEST(ShapeTests, TwoSegmentShapeWalking)
{
  //M7::WVShape saw = {.mSegments = {
  //                       M7::WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = 1, .beginAmp = -1.0f, .slope = +2.0f},
  //                   }};

  //{
  //  auto walker = saw.MakeWalker(0);
  //  int encounters = 0;

  //  auto visitEdge = [&](size_t segmentIndex,
  //                       const M7::WVSegment& edge,
  //                       float preA,
  //                       float preS,
  //                       float postA,
  //                       float postS,
  //                       double distFromPathStartToEdgeInPhase01
  //      //,std::string s
  //      )
  //  {
  //    cc::log(
  //        std::format("@ {:.3f}, enter segment #{}, left edge @ {:.3f} phase; pre[{:.3f}, {:.3f}] -> [{:.3f}, {:.3f}]",
  //                    walker.mCursorPhase01,
  //                    segmentIndex,
  //                    edge.beginPhase01,
  //                    preA,
  //                    preS,
  //                    postA,
  //                    postS));
  //    encounters++;
  //  };

  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));
  //  walker.Step(0.5, visitEdge);
  //  EXPECT_EQ(encounters, 1);
  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));
  //  walker.Step(0.5, visitEdge);
  //  EXPECT_EQ(encounters, 1);  // landing directly on next edge will eval after.
  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));
  //  walker.Step(0.5, visitEdge);
  //  EXPECT_EQ(encounters, 2);
  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));
  //  walker.Step(0.5, visitEdge);
  //  EXPECT_EQ(encounters, 2);
  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));
  //  walker.Step(0.5, visitEdge);
  //  EXPECT_EQ(encounters, 3);
  //  cc::log(std::format("walking from {:.3f}", walker.mCursorPhase01));
  //  walker.Step(0.5, visitEdge);
  //  EXPECT_EQ(encounters, 3);
  //}
}
