cmake_minimum_required(VERSION 3.11)
project("WaveSabre" LANGUAGES C CXX CSharp)

# Explicit C++ standard for all targets
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CTest) # provides BUILD_TESTING option and CTest integration

set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo;MinSizeRel;MinSizeRelWithDebugInfo" CACHE STRING "" FORCE)

set(TOOLS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Tools CACHE STRING "" FORCE)

# Enable C# Utilities also for the VS2013 AppVeyor stage.
cmake_policy(SET CMP0057 NEW)
include(CSharpUtilities)
set(CMAKE_CSharp_FLAGS "/langversion:7")
set_property(GLOBAL PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2")


# define SELECTABLE_OUTPUT_STREAM_SUPPORT and FFT_ANALYSIS_SUPPORT for non-MinSizeRel configs
add_compile_definitions(
	$<$<AND:$<NOT:$<CONFIG:MinSizeRel>>,$<NOT:$<CONFIG:MinSizeRelWithDebugInfo>>>:SELECTABLE_OUTPUT_STREAM_SUPPORT>
	$<$<AND:$<NOT:$<CONFIG:MinSizeRel>>,$<NOT:$<CONFIG:MinSizeRelWithDebugInfo>>>:FFT_ANALYSIS_SUPPORT>
)

# MSVC MinSizeRel quirk: avoid sized delete calls that would pull in vcruntime
if(MSVC)
	add_compile_options($<$<OR:$<CONFIG:MinSizeRel>,$<CONFIG:MinSizeRelWithDebugInfo>>:/Zc:sizedDealloc->)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
	# disable exceptions globally (will be added back for VSTs)
	string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endif()

set(BUILD_VST_PLUGINS ON CACHE BOOL "Opt out of building the VST plugins.")
set(BUILD_WAVESABRE_CONVERT ON CACHE BOOL "Do not build WaveSabreConvert.")
set(BUILD_CONVERT_THE_FUCK ON CACHE BOOL "Do not build ConvertTheFuck.")
set(BUILD_PROJECT_MANAGER ON CACHE BOOL "Do not build the project manager.")
set(VSTSDK3_DIR "${PROJECT_SOURCE_DIR}/Vst3.x/" CACHE PATH "VSTSDK location")

# Download and unpack VST3 SDK
set(DOWNLOAD_VST3SDK OFF CACHE BOOL "Download and unpack VST3 SDK automatically.")
if(${BUILD_VST_PLUGINS} AND ${DOWNLOAD_VST3SDK})
	find_file(VST3SDK_TEST name public.sdk HINTS ${VSTSDK3_DIR})
	if(${VST3SDK_TEST} MATCHES VST3SDK_TEST-NOTFOUND)
		message(STATUS "VST3 SDK not found. Will download.")
		file(DOWNLOAD https://web.archive.org/web/20200502121517/https://www.steinberg.net/sdk_downloads/vstsdk366_27_06_2016_build_61.zip vstsdk366_27_06_2016_build_61.zip SHOW_PROGRESS)
		file(ARCHIVE_EXTRACT INPUT vstsdk366_27_06_2016_build_61.zip DESTINATION vstsdk366_27_06_2016_build_61 VERBOSE)
		file(GLOB VST3SDK_FILES "${CMAKE_CURRENT_BINARY_DIR}/vstsdk366_27_06_2016_build_61/VST3\ SDK/*")
		file(COPY ${VST3SDK_FILES} DESTINATION ${VSTSDK3_DIR})
	endif()
endif()

# set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /arch:IA32") # avoids modern CRT calls; however at least on vs2022 this says unknown compile option.
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /fp:fast") # avoids modern CRT calls
set(CMAKE_C_FLAGS_MINSIZERELWITHDEBUGINFO "${CMAKE_C_FLAGS_MINSIZEREL}")
set(CMAKE_CXX_FLAGS_MINSIZERELWITHDEBUGINFO "${CMAKE_CXX_FLAGS_MINSIZEREL} /Zi")
# It's tempting to add PDB output for all configs, however it's already on for RelWithDebInfo and Debug.
# For MinSizeRel, don't output it because it adds ~100 bytes to the resulting compressed binary.
# set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /GL") # whole program optimization (see LTCG set at the target makefile)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Ot") # favor fast code
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Oi") # enable intrinsics
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Ob2") # enable inlining for any suitable
if(MSVC)
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /INCREMENTAL:NO")
	set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO} /INCREMENTAL:NO")
	set(CMAKE_EXE_LINKER_FLAGS_MINSIZERELWITHDEBUGINFO "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /DEBUG /DEBUG:FULL /INCREMENTAL:NO")
	set(CMAKE_SHARED_LINKER_FLAGS_MINSIZERELWITHDEBUGINFO "${CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL} /DEBUG /DEBUG:FULL /INCREMENTAL:NO")
endif()

add_compile_definitions($<$<OR:$<CONFIG:MinSizeRel>,$<CONFIG:MinSizeRelWithDebugInfo>>:MIN_SIZE_REL>) # a lot of special cases for size-optimization and CRT avoidance; make it easy.
add_compile_definitions(NOMINMAX) # prevent windows.h from invasive #defines
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_compile_definitions(WINVER=0x0A00)
add_compile_definitions(_WIN32_WINNT=0x0A00)

add_compile_definitions(IMGUI_DEFINE_MATH_OPERATORS)

# shared code
add_subdirectory(MSVCRT)
add_subdirectory(WaveSabreCore)
add_subdirectory(WaveSabrePlayerLib)

# binaries
add_subdirectory(WaveSabreStandAlonePlayer)
add_subdirectory(WaveSabreExecutableMusicPlayer)
add_subdirectory(Maj7ConsolePlayer)

# VSTs
set(VST_TARGETS "" CACHE INTERNAL "")
if(${BUILD_VST_PLUGINS} AND VSTSDK3_DIR)
	add_subdirectory(WaveSabreVstLib)
	add_subdirectory(Vsts)
	# ImGui-based manual test harness
	add_subdirectory(WaveSabreManualTests)
endif()

# Project file conversions
if(${BUILD_WAVESABRE_CONVERT})
	add_subdirectory(WaveSabreConvert)
endif()
if(${BUILD_CONVERT_THE_FUCK})
	add_subdirectory(ConvertTheFuck)
endif()

# Project manager
if(${BUILD_PROJECT_MANAGER})
	add_subdirectory(ProjectManager)
endif()

# Unit tests (GoogleTest)
if(BUILD_TESTING)
	add_subdirectory(WaveSabreCoreTests)
endif()
