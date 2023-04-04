add_library(WaveSabreCore
	include/WaveSabreCore/Adultery.h
	include/WaveSabreCore/AllPass.h
	include/WaveSabreCore/AllPassDelay.h
	include/WaveSabreCore/BiquadFilter.h
	include/WaveSabreCore/Cathedral.h
	include/WaveSabreCore/Chamber.h
	include/WaveSabreCore/Comb.h
	include/WaveSabreCore/Crusher.h
	include/WaveSabreCore/DelayBuffer.h
	include/WaveSabreCore/Device.h
	include/WaveSabreCore/Devices.h
	include/WaveSabreCore/Echo.h
	include/WaveSabreCore/Envelope.h
	include/WaveSabreCore/Falcon.h
	include/WaveSabreCore/GmDls.h
	include/WaveSabreCore/GsmSample.h
	include/WaveSabreCore/Helpers.h
	include/WaveSabreCore/Leveller.h
	include/WaveSabreCore/Maj7.hpp
	include/WaveSabreCore/Maj7SynthDevice.hpp
	include/WaveSabreCore/Maj7Basic.hpp
	include/WaveSabreCore/Maj7Envelope.hpp
	include/WaveSabreCore/Maj7Filter.hpp
	include/WaveSabreCore/Maj7Bitcrush.hpp
	include/WaveSabreCore/Maj7Distortion.hpp
	include/WaveSabreCore/Maj7ModMatrix.hpp
	include/WaveSabreCore/Maj7Oscillator.hpp
	include/WaveSabreCore/Maj7Sampler.hpp
	include/WaveSabreCore/Maj7GmDls.hpp
	include/WaveSabreCore/Maj7ParamAccessor.hpp
	include/WaveSabreCore/Maj7Width.hpp
	include/WaveSabreCore/MxcsrFlagGuard.h
	include/WaveSabreCore/ResampleBuffer.h
	include/WaveSabreCore/SamplePlayer.h
	include/WaveSabreCore/Scissor.h
	include/WaveSabreCore/Slaughter.h
	include/WaveSabreCore/Smasher.h
	include/WaveSabreCore/Specimen.h
	include/WaveSabreCore/StateVariableFilter.h
	include/WaveSabreCore/SynthDevice.h
	include/WaveSabreCore/Twister.h
	include/WaveSabreCore/Deprecated/Thunder.h
	include/WaveSabreCore/Filters/FilterSEM12.hpp
	include/WaveSabreCore/Filters/FilterBase.hpp
	include/WaveSabreCore/Filters/FilterDiode.hpp
	include/WaveSabreCore/Filters/FilterK35.hpp
	include/WaveSabreCore/Filters/FilterMoog.hpp
	include/WaveSabreCore/Filters/FilterOnePole.hpp
	src/Adultery.cpp
	src/AllPass.cpp
	src/AllPassDelay.cpp
	src/BiquadFilter.cpp
	src/Cathedral.cpp
	src/Chamber.cpp
	src/Comb.cpp
	src/Crusher.cpp
	src/DelayBuffer.cpp
	src/Device.cpp
	src/Echo.cpp
	src/Envelope.cpp
	src/Falcon.cpp
	src/GmDls.cpp
	src/GsmSample.cpp
	src/Helpers.cpp
	src/Leveller.cpp
	src/Maj7.cpp
	src/Maj7Basic.cpp
	src/Maj7Envelope.cpp
	src/Maj7ModMatrix.cpp
	src/Maj7Oscillator.cpp
	src/Maj7ParamAccessor.cpp
	src/Maj7Sampler.cpp
	src/Maj7SynthDevice.cpp
	src/MxcsrFlagGuard.cpp
	src/ResampleBuffer.cpp
	src/SamplePlayer.cpp
	src/Scissor.cpp
	src/Slaughter.cpp
	src/Smasher.cpp
	src/Specimen.cpp
	src/StateVariableFilter.cpp
	src/SynthDevice.cpp
	src/Twister.cpp
	src/Deprecated/Thunder.cpp)

target_link_libraries(WaveSabreCore Msacm32.lib)
target_include_directories(WaveSabreCore PUBLIC include)
target_compile_definitions(WaveSabreCore PUBLIC NOMINMAX)

if(MSVC)
	target_compile_options(WaveSabreCore
		PUBLIC $<$<CONFIG:MinSizeRel>:/GS- /GL /Gy /Zi /GR->  # /GL is whole program optimization and causes some issues sometimes.
		PRIVATE $<$<CONFIG:MinSizeRel>:/Oi /fp:fast>)

	target_compile_definitions(WaveSabreCore PRIVATE $<$<CONFIG:MinSizeRel>:MIN_SIZE_REL>)

	set_property(TARGET WaveSabreCore APPEND_STRING PROPERTY STATIC_LIBRARY_FLAGS_MINSIZEREL
		" /LTCG")

	if(MSVC_VERSION GREATER 1900)
		target_compile_options(WaveSabreCore PUBLIC
			$<$<CONFIG:MinSizeRel>:/Zc:sizedDealloc->)
	endif()
endif()
