
file(GLOB_RECURSE HEADER_FILES 
    *.hpp
    *.h
)

file(GLOB_RECURSE SOURCE_FILES 
    *.cpp
)

set(ALL_FILES ${HEADER_FILES} ${SOURCE_FILES})

# group files by real filesystem hierarchy in Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ALL_FILES})

add_library(WaveSabreCore ${ALL_FILES})

target_link_libraries(WaveSabreCore Msacm32.lib)
target_include_directories(WaveSabreCore PUBLIC include)

if(MSVC)
	target_compile_options(WaveSabreCore
		PUBLIC $<$<OR:$<CONFIG:MinSizeRel>,$<CONFIG:MinSizeRelWithDebugInfo>>:/GS- /GL /Gy /Zi /GR->  # /GL is whole program optimization and causes some issues sometimes.
		PRIVATE $<$<OR:$<CONFIG:MinSizeRel>,$<CONFIG:MinSizeRelWithDebugInfo>>:/Oi /fp:fast>)

	set_property(TARGET WaveSabreCore APPEND_STRING PROPERTY STATIC_LIBRARY_FLAGS_MINSIZEREL
		" /LTCG")
	set_property(TARGET WaveSabreCore APPEND_STRING PROPERTY STATIC_LIBRARY_FLAGS_MINSIZERELWITHDEBUGINFO
		" /LTCG")

	# MSVC MinSizeRel quirk: avoid sized delete calls that would pull in vcruntime
	# if(MSVC_VERSION GREATER 1900)
	# 	target_compile_options(WaveSabreCore PUBLIC
	# 		$<$<CONFIG:MinSizeRel>:/Zc:sizedDealloc->)
	# endif()
endif()
