// https://www.musicdsp.org/en/latest/Effects/114-waveshaper-simple-description.html
// https://www.musicdsp.org/en/latest/Effects/46-waveshaper.html

#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include "Maj7ModMatrix.hpp"
//
//namespace WaveSabreCore
//{
//	namespace M7
//	{
//        static constexpr float gDistortionAuxDriveMaxDb = 24;
//
//		enum class DistortionStyle : uint8_t
//		{
//            HardClip,
//            SoftClip,
//            Triangle,
//            Sine,
//            Hyperbolic,
//            Rectify,
//            Type1,
//            Count
//		};
//
//        // size-optimize using macro
//#define DISTORTION_STYLE_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::DistortionStyle::Count] { \
//            "HardClip", \
//            "SoftClip", \
//            "Triangle", \
//            "Sine", \
//            "Hyperbolic", \
//            "Rectify", \
//            "Type1", \
//        }
//
//        struct DistortionAuxNode : IAuxEffect
//        {
//            EnumParam<DistortionStyle> mStyle;
//            VolumeParam mDrive;
//            Float01Param mThreshold;
//            Float01Param mShape;
//            VolumeParam mOutputVolume;
//
//            int mModDestParam2ID;
//            DCFilter mDCFilter;
//
//            // calc
//            float mLiveInputMul = 0;
//            float mLiveThreshold = 0;
//            float mLiveShape = 0;
//            float mLiveOutputMul = 0;
//
//            DistortionAuxNode(float* auxParams, int modDestParam2ID) :
//                // !! do not SET initial values; these get instantiated dynamically.
//                mStyle(auxParams[(int)DistortionAuxParamIndexOffsets::DistortionStyle], DistortionStyle::Count),
//                mDrive(auxParams[(int)DistortionAuxParamIndexOffsets::Drive], gDistortionAuxDriveMaxDb),
//                mThreshold(auxParams[(int)DistortionAuxParamIndexOffsets::Threshold]),
//                mShape(auxParams[(int)DistortionAuxParamIndexOffsets::Shape]),
//                mOutputVolume(auxParams[(int)DistortionAuxParamIndexOffsets::OutputVolume], 0),
//                mModDestParam2ID(modDestParam2ID)
//            {
//            }
//
//            virtual void AuxBeginBlock(float noteHz, int nSamples, ModMatrixNode& modMatrix) override
//            {
//                mLiveInputMul = mDrive.GetLinearGain(modMatrix.GetDestinationValue(mModDestParam2ID + (int)DistortionAuxModIndexOffsets::Drive, 0));
//                mLiveThreshold = mThreshold.Get01Value(modMatrix.GetDestinationValue(mModDestParam2ID + (int)DistortionAuxModIndexOffsets::Threshold, 0));
//                if (mLiveThreshold < 0.001f) {
//                    mLiveThreshold = 0.001f;
//                }
//                mLiveShape = mShape.Get01Value(modMatrix.GetDestinationValue(mModDestParam2ID + (int)DistortionAuxModIndexOffsets::Shape, 0));
//                mLiveOutputMul = mOutputVolume.GetLinearGain(modMatrix.GetDestinationValue(mModDestParam2ID + (int)DistortionAuxModIndexOffsets::OutputVolume, 0));
//            }
//
//            virtual float AuxProcessSample(float s) override
//            {
//                float ret = ProcessSample(s, mStyle.GetEnumValue(), mLiveInputMul, mLiveThreshold, mLiveShape, mLiveOutputMul);
//                return mDCFilter.ProcessSample(ret);
//            }
//
//            // t is periodic 0,1
//            // returns -1,1
//            static float Triangle(float t)
//            {
//                t = Fract(t + 0.5f); // 0,1. the +.5 is so the output corresponds to 
//                t = t - 0.5f; // -.5, +.5
//                t = std::abs(t); // 0.5 -> 0 -> 0.5
//                t = t - 0.25f; // .25 -> -.25 -> .25
//                return t * 4;
//            }
//
//            static float ProcessSample(float s, DistortionStyle style, float mLiveInputMul, float mLiveThreshold, float mLiveShape, float mLiveOutputMul)
//            {
//                switch (style)
//                {
//                case DistortionStyle::HardClip:
//                {
//                    // inpvol & live thresh are redundant.
//                    //s /= mLiveThreshold; // scale so thresh is -1,1
//                    //s = Clamp(s, -1, 1); // eliminates foldback tho
//                    s *= mLiveInputMul;
//                    float mul = 1;
//                    float a = std::abs(s);
//                    //if (a > 1) {
//                    //    mul = 1 / a * mLiveShape * 10;
//                    //}
//                    mul = 1 / std::max(1.0f, a * mLiveShape * 5);
//                    s = Triangle(s * 0.5f);
//                    s *= mul;
//
//                    // everything above 1 attenuate 
//
//                    s /= mLiveThreshold; // scale so thresh is -1,1
//                    s = Clamp(s, -1, 1); // eliminates foldback tho
//
//                    //if (s < -1.0f) {
//                    //    s = -1.0f + (-1.0f - s) * mLiveShape;
//                    //}
//                    //else if (s > 1.0f) {
//                    //    s = 1.0f + (1.0f - s) * mLiveShape;
//                    //}
//                    break;
//                }
//                case DistortionStyle::Triangle:
//                {
//                    // inpvol & live thresh are redundant.
//                    //s /= mLiveThreshold; // scale so thresh is -1,1
//                    //s = Clamp(s, -1, 1); // eliminates foldback tho
//                    s *= mLiveInputMul;
//                    float mul = 1;
//                    float a = std::abs(s);
//                    //if (a > 1) {
//                    //    mul = 1 / a * mLiveShape * 10;
//                    //}
//                    mul = 1 / std::max(1.0f, a * mLiveShape * 5);
//                    s = Triangle(s * 0.5f);
//                    s *= mul;
//
//                    // everything above 1 attenuate 
//
//                    s /= mLiveThreshold; // scale so thresh is -1,1
//                    s = Clamp(s, -1, 1); // eliminates foldback tho
//
//                    //if (s < -1.0f) {
//                    //    s = -1.0f + (-1.0f - s) * mLiveShape;
//                    //}
//                    //else if (s > 1.0f) {
//                    //    s = 1.0f + (1.0f - s) * mLiveShape;
//                    //}
//                    break;
//                }
//                case DistortionStyle::Sine:
//                {
//                    // inpvol is like drive
//                    // thresh is hard clipping after sine.
//                    s *= mLiveInputMul;
//                    s = math::sin(s);
//                    s *= (1 + s * mLiveShape * 2); // i dunno this is kinda lame right?
//                    s /= mLiveThreshold;
//                    s = Clamp(s, -1, 1);
//                    break;
//                }
//                case DistortionStyle::SoftClip:
//                {
//                    s *= mLiveInputMul;
//                    s /= mLiveThreshold;
//                    s = Clamp(s, -1, 1);
//                    s = modCurve_xN11_kN11(s, mLiveShape * 2 - 1); // invokes 4 pow() calls; i'd like to find an alternative although this sounds pretty dope
//                    break;
//                }
//                case DistortionStyle::Hyperbolic:
//                {
//                    s *= mLiveInputMul;
//                    s /= mLiveThreshold;
//                    s += mLiveShape;
//                    s = Clamp(s, -1, 1);
//                    s = math::tanh(s * 10.0f);
//                    break;
//                }
//                case DistortionStyle::Rectify:
//                {
//                    s *= mLiveInputMul;
//                    // shape = offset
//                    s /= mLiveThreshold;
//                    s += mLiveShape;
//                    s = Clamp(s, -1, 1);
//                    s = std::abs(s);
//                    s -= 0.5f; // DC correct?
//                    break;
//                }
//                case DistortionStyle::Type1:
//                {
//                    // slider1:20<0,50,1>Gain (dB)
//                    // slider2:6<1,10,1>Hardness
//                    // slider3:-12<-40,0,1>Max Volume (dB)
//                    // slider4:0<0,2,1{Left,Right,Stereo}>Channel Mode
//                    // 
//                    // preamp=2^(slider1/6);
//                    // soft=2^slider2;
//                    // maxv=2 ^ (slider3/6);
//                    // 
//                    // spl0*=preamp;
//                    // t=abs(spl0);
//                    // t > maxv ?
//                    // (
//                    //   s=sign(spl0);
//                    //   diff=t-maxv;
//                    //   spl0=s*(maxv + diff/(soft+diff));
//                    // );
//                    break;
//                }
//
//                default:
//                    break;
//                }
//
//                s *= mLiveOutputMul;
//                s = Clamp(s, -1, 1);
//                return s;
//            }
//
//        };
//
//	} // namespace M7
//
//
//} // namespace WaveSabreCore
//
//
//
//
