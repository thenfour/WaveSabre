#pragma once

#include "Device.h"
#include <vector>
#include <complex>
#include <cmath>

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

#include "RMS.hpp" // for base analysis stream stuff
#include "FFTAnalysis.hpp"

namespace WaveSabreCore
{
    // Stereo imaging analysis stream for real-time stereo field visualization
    struct StereoImagingAnalysisStream {
        RMSDetector mLeftLevelDetector;   // *** LEFT CHANNEL RMS ***
        RMSDetector mRightLevelDetector;  // *** RIGHT CHANNEL RMS ***
        RMSDetector mWidthDetector;       // For stereo width RMS

        AnalysisStream mMidLevelDetector;
        AnalysisStream mSideLevelDetector;

        // Current analysis values (professionally smoothed)
        double mPhaseCorrelation = 0.0;  // -1 to +1, -1 = out of phase, 0 = uncorrelated, +1 = mono
        double mStereoWidth = 0.0;       // 0 = mono, 1 = normal stereo, >1 = wide  
        double mStereoBalance = 0.0;     // -1 to +1, -1 = full left, 0 = center, +1 = full right
        double mLeftLevel = 0.0;         // *** LEFT CHANNEL RMS LEVEL ***
        double mRightLevel = 0.0;        // *** RIGHT CHANNEL RMS LEVEL ***

        // Smoothing filters for display stability
        static constexpr double gSmoothingFactor = 0.8; // For correlation (sample-accurate calculation)

        // History buffer for phase scope visualization  
        static constexpr size_t gHistorySize = 256;
        struct StereoSample {
            float left = 0.0f;
            float right = 0.0f;
        };
        StereoSample mHistory[gHistorySize];
        size_t mHistoryIndex = 0;

        // Correlation calculation helpers
        double mLeftSum = 0.0;
        double mRightSum = 0.0;
        double mLeftSquaredSum = 0.0;
        double mRightSquaredSum = 0.0;
        double mLRProductSum = 0.0;
        size_t mCorrelationSampleCount = 0;
        static constexpr size_t gCorrelationWindowSize = 4800; // ~100ms at 48kHz

        explicit StereoImagingAnalysisStream() {
            // Configure professional RMS detectors with appropriate time constants
            mLeftLevelDetector.SetWindowSize(200);
            mRightLevelDetector.SetWindowSize(200);
            mWidthDetector.SetWindowSize(200);
            Reset();
        }

        void WriteStereoSample(double left, double right) {
            // Update history buffer for phase scope
            mHistory[mHistoryIndex].left = static_cast<float>(left);
            mHistory[mHistoryIndex].right = static_cast<float>(right);
            mHistoryIndex = (mHistoryIndex + 1) % gHistorySize;

            // *** PROCESS L/R CHANNELS WITH DEDICATED RMS DETECTORS ***
            mLeftLevel = mLeftLevelDetector.ProcessSample(left);   // Direct L/R RMS
            mRightLevel = mRightLevelDetector.ProcessSample(right); // No abs() needed - RMS detector handles it internally

            // Calculate mid-side components
            double mid = (left + right) * 0.5;
            double side = (left - right) * 0.5;

            mMidLevelDetector.WriteSample(mid);
            mSideLevelDetector.WriteSample(side);

            if (mMidLevelDetector.mCurrentPeak > 0.0001) {
                double instantWidth = mSideLevelDetector.mCurrentPeak / mMidLevelDetector.mCurrentPeak;
                mStereoWidth = mWidthDetector.ProcessSample(instantWidth);
            }

            double totalLevel = mLeftLevel + mRightLevel;
            if (totalLevel > 0.0001) {
                mStereoBalance = (mRightLevel - mLeftLevel) / totalLevel;
            }

            // Update correlation calculation (keep existing sample-accurate method)
            mLeftSum += left;
            mRightSum += right;
            mLeftSquaredSum += left * left;
            mRightSquaredSum += right * right;
            mLRProductSum += left * right;
            mCorrelationSampleCount++;

            // Calculate correlation periodically to avoid overflow and keep it current
            if (mCorrelationSampleCount >= gCorrelationWindowSize) {
                UpdateCorrelation();
                // Reset accumulators but keep some history for stability
                mLeftSum *= 0.5;
                mRightSum *= 0.5;
                mLeftSquaredSum *= 0.5;
                mRightSquaredSum *= 0.5;
                mLRProductSum *= 0.5;
                mCorrelationSampleCount /= 2;
            }
        }

        void Reset() {
            mPhaseCorrelation = 0.0;
            mStereoWidth = 0.0;
            mStereoBalance = 0.0;
            mLeftLevel = 0.0;
            mRightLevel = 0.0;
            mHistoryIndex = 0;

            // Reset professional detectors
            mLeftLevelDetector.Reset();
            mRightLevelDetector.Reset();
            mMidLevelDetector.Reset();
            mSideLevelDetector.Reset();
            mWidthDetector.Reset();

            for (auto& sample : mHistory) {
                sample.left = sample.right = 0.0f;
            }

            mLeftSum = mRightSum = 0.0;
            mLeftSquaredSum = mRightSquaredSum = 0.0;
            mLRProductSum = 0.0;
            mCorrelationSampleCount = 0;
        }

        // Get the current history buffer for visualization
        const StereoSample* GetHistoryBuffer() const { return mHistory; }
        size_t GetHistorySize() const { return gHistorySize; }
        size_t GetCurrentHistoryIndex() const { return mHistoryIndex; }

    private:
        void UpdateCorrelation() {
            if (mCorrelationSampleCount < 2) {
                mPhaseCorrelation = 0.0;
                return;
            }

            double n = static_cast<double>(mCorrelationSampleCount);

            // Calculate means
            double leftMean = mLeftSum / n;
            double rightMean = mRightSum / n;

            // Calculate variances and covariance
            double leftVariance = (mLeftSquaredSum / n) - (leftMean * leftMean);
            double rightVariance = (mRightSquaredSum / n) - (rightMean * rightMean);
            double covariance = (mLRProductSum / n) - (leftMean * rightMean);

            // Calculate correlation coefficient
            double denominator = std::sqrt(leftVariance * rightVariance);
            if (denominator > 0.0001) {
                double instantCorrelation = covariance / denominator;
                // Clamp to valid range and apply smoothing
                instantCorrelation = std::max(-1.0, std::min(1.0, instantCorrelation));
                mPhaseCorrelation = mPhaseCorrelation * gSmoothingFactor + instantCorrelation * (1.0 - gSmoothingFactor);
            }
        }
    };
    // 
}

#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
