set(CMAKE_WIN32_EXECUTABLE ON)
add_executable(WaveSabreExecutableMusicPlayer WIN32
	rendered.hpp
	main.cpp)

target_link_libraries(WaveSabreExecutableMusicPlayer WaveSabrePlayerLib)

if(MSVC)
	target_compile_definitions(WaveSabreExecutableMusicPlayer PRIVATE _CRT_SECURE_NO_WARNINGS)
	
	target_compile_options(WaveSabreExecutableMusicPlayer
		PUBLIC $<$<CONFIG:MinSizeRel>:/GS- /GL /Gy /Zi /GR->  # /GL is whole program optimization and causes some issues sometimes.
		PRIVATE $<$<CONFIG:MinSizeRel>:/Oi /fp:fast>) # explore /d2noftol3

	target_link_libraries(WaveSabreExecutableMusicPlayer $<$<CONFIG:MinSizeRel>:msvcrt>)
	set_property(TARGET WaveSabreExecutableMusicPlayer APPEND_STRING PROPERTY LINK_FLAGS_MINSIZEREL
		" /NODEFAULTLIB /SAFESEH:NO /MANIFEST:NO /LTCG /OPT:REF /OPT:ICF /DYNAMICBASE:NO")

	if(MSVC_VERSION GREATER 1900)
		target_compile_definitions(WaveSabreExecutableMusicPlayer PRIVATE
			$<$<CONFIG:MinSizeRel>:_NO_CRT_STDIO_INLINE>)
	endif()

	if(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
		add_custom_command(TARGET WaveSabreExecutableMusicPlayer POST_BUILD
			COMMAND ${TOOLS_PATH}/squishy-x86.exe -i $<TARGET_FILE:WaveSabreExecutableMusicPlayer> -o $<TARGET_FILE_DIR:WaveSabreExecutableMusicPlayer>/packed.exe
			COMMENT "Packing 32-bit executable")
	else()
		add_custom_command(TARGET WaveSabreExecutableMusicPlayer POST_BUILD
			COMMAND ${TOOLS_PATH}/squishy-x64.exe -i $<TARGET_FILE:WaveSabreExecutableMusicPlayer> -o $<TARGET_FILE_DIR:WaveSabreExecutableMusicPlayer>/packed.exe
			COMMENT "Packing 64-bit executable")
  	endif()

endif()
