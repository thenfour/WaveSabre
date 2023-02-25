add_executable(WaveSabreExecutableMusicPlayer main.cpp)
target_link_libraries(WaveSabreExecutableMusicPlayer WaveSabrePlayerLib)

if(MSVC)
	target_compile_definitions(WaveSabreExecutableMusicPlayer PRIVATE _CRT_SECURE_NO_WARNINGS)

	target_link_libraries(WaveSabreExecutableMusicPlayer $<$<CONFIG:MinSizeRel>:msvcrt>)
	set_property(TARGET WaveSabreExecutableMusicPlayer APPEND_STRING PROPERTY LINK_FLAGS_MINSIZEREL
		" /NODEFAULTLIB /SAFESEH:NO /MANIFEST:NO /LTCG /OPT:REF /OPT:ICF /DYNAMICBASE:NO")

	if(MSVC_VERSION GREATER 1900)
		target_compile_definitions(WaveSabreExecutableMusicPlayer PRIVATE
			$<$<CONFIG:MinSizeRel>:_NO_CRT_STDIO_INLINE>)
	endif()
endif()
