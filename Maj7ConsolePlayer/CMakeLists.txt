add_executable(Maj7ConsolePlayer
	main.cpp
	../WaveSabreExecutableMusicPlayer/rendered.hpp)

target_link_libraries(Maj7ConsolePlayer WaveSabrePlayerLib)

if(MSVC)
	target_compile_definitions(Maj7ConsolePlayer PRIVATE _CRT_SECURE_NO_WARNINGS)
	target_compile_options(Maj7ConsolePlayer
		PRIVATE $<$<OR:$<CONFIG:MinSizeRel>,$<CONFIG:MinSizeRelWithDebugInfo>>:/d2noftol3>)

	target_link_libraries(Maj7ConsolePlayer $<$<OR:$<CONFIG:MinSizeRel>,$<CONFIG:MinSizeRelWithDebugInfo>>:msvcrt>)
	set_property(TARGET Maj7ConsolePlayer APPEND_STRING PROPERTY LINK_FLAGS_MINSIZEREL
		" /NODEFAULTLIB /SAFESEH:NO /MANIFEST:NO /LTCG /OPT:REF /OPT:ICF /DYNAMICBASE:NO")
	set_property(TARGET Maj7ConsolePlayer APPEND_STRING PROPERTY LINK_FLAGS_MINSIZERELWITHDEBUGINFO
		" /NODEFAULTLIB /SAFESEH:NO /MANIFEST:NO /LTCG /OPT:REF /OPT:ICF /DYNAMICBASE:NO")

	if(MSVC_VERSION GREATER 1900)
		target_compile_definitions(Maj7ConsolePlayer PRIVATE
			$<$<OR:$<CONFIG:MinSizeRel>,$<CONFIG:MinSizeRelWithDebugInfo>>:_NO_CRT_STDIO_INLINE>)
	endif()

	if(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
		add_custom_command(TARGET Maj7ConsolePlayer POST_BUILD
			COMMAND ${TOOLS_PATH}/squishy-x86.exe -i $<TARGET_FILE:Maj7ConsolePlayer> -o $<TARGET_FILE_DIR:Maj7ConsolePlayer>/packed.exe
			COMMENT "Packing 32-bit executable")
	else()
		add_custom_command(TARGET Maj7ConsolePlayer POST_BUILD
			COMMAND ${TOOLS_PATH}/squishy-x64.exe -i $<TARGET_FILE:Maj7ConsolePlayer> -o $<TARGET_FILE_DIR:Maj7ConsolePlayer>/packed.exe
			COMMENT "Packing 64-bit executable")
  	endif()

endif()
