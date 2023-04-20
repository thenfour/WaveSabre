add_library(WaveSabrePlayerLib
	include/WaveSabrePlayerLib/PreRenderPlayer.h
	include/WaveSabrePlayerLib/WavWriter.h
	include/WaveSabrePlayerLib/DirectSoundRenderThread.h
	include/WaveSabrePlayerLib/RealtimePlayer.h
	include/WaveSabrePlayerLib/IPlayer.h
	include/WaveSabrePlayerLib/SongRenderer.h
	include/WaveSabrePlayerLib/SongRenderer2.hpp
	src/DirectSoundRenderThread.cpp
	src/IPlayer.cpp
	src/PreRenderPlayer.cpp
	src/RealtimePlayer.cpp
	src/SongRenderer.cpp
	src/SongRenderer.Track.cpp
	src/WavWriter.cpp)

target_link_libraries(WaveSabrePlayerLib
	WaveSabreCore
	winmm.lib
	dsound.lib)

target_include_directories(WaveSabrePlayerLib PUBLIC include)

if(MSVC)
	target_compile_definitions(WaveSabrePlayerLib PRIVATE _CRT_SECURE_NO_WARNINGS)
	target_compile_definitions(WaveSabrePlayerLib PRIVATE $<$<CONFIG:MinSizeRel>:MIN_SIZE_REL>)
	target_compile_options(WaveSabrePlayerLib PUBLIC
		$<$<CONFIG:MinSizeRel>:/EHs-c->)
	set_property(TARGET WaveSabrePlayerLib APPEND_STRING PROPERTY STATIC_LIBRARY_FLAGS_MINSIZEREL
		" /LTCG")
endif()
