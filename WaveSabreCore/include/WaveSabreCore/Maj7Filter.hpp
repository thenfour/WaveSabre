#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>

// TODO: comb & notch

namespace WaveSabreCore
{
	namespace M7
	{
		struct FilterNode
		{
			void BeginBlock() {

			}
			real_t ProcessSample(real_t inputSample)
			{
				return inputSample;
			}
		};
	} // namespace M7


} // namespace WaveSabreCore







