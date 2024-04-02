
file(GLOB_RECURSE HEADER_FILES 
    include/WaveSabreCore/*.hpp
    include/WaveSabreCore/*.h
)

file(GLOB_RECURSE SOURCE_FILES 
    src/*.cpp
)

set(ALL_FILES ${HEADER_FILES} ${SOURCE_FILES})

# group all files under the "Source Files" folder in Visual Studio
source_group("Source Files" FILES ${ALL_FILES})

add_library(WaveSabreCore ${ALL_FILES})

target_link_libraries(WaveSabreCore Msacm32.lib)
target_include_directories(WaveSabreCore PUBLIC include)

if(MSVC)
	target_compile_options(WaveSabreCore
		PUBLIC $<$<CONFIG:MinSizeRel>:/GS- /GL /Gy /Zi /GR->  # /GL is whole program optimization and causes some issues sometimes.
		PRIVATE $<$<CONFIG:MinSizeRel>:/Oi /fp:fast>)

	set_property(TARGET WaveSabreCore APPEND_STRING PROPERTY STATIC_LIBRARY_FLAGS_MINSIZEREL
		" /LTCG")

	if(MSVC_VERSION GREATER 1900)
		target_compile_options(WaveSabreCore PUBLIC
			$<$<CONFIG:MinSizeRel>:/Zc:sizedDealloc->)
	endif()
endif()
