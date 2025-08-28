file(GLOB_RECURSE WAVE_SABRE_VSTLIB_HEADERS
    "include/WaveSabreVstLib/*.h"
    "include/WaveSabreVstLib/*.hpp"
)
file(GLOB_RECURSE WAVE_SABRE_VSTLIB_SOURCES
    "src/*.cpp"
    "src/*.c"
)
file(GLOB WAVE_SABRE_VSTLIB_7Z
	"${CMAKE_SOURCE_DIR}/third_party/7z/*.h"
	"${CMAKE_SOURCE_DIR}/third_party/7z/*.c"
)

add_library(WaveSabreVstLib STATIC
	${VSTSDK3_DIR}/public.sdk/source/vst2.x/audioeffect.cpp
	${VSTSDK3_DIR}/public.sdk/source/vst2.x/audioeffectx.cpp
	${VSTSDK3_DIR}/public.sdk/source/vst2.x/vstplugmain.cpp
    ${WAVE_SABRE_VSTLIB_HEADERS}
    ${WAVE_SABRE_VSTLIB_SOURCES}
    ${WAVE_SABRE_VSTLIB_7Z}
	${CMAKE_SOURCE_DIR}/third_party/imgui/imconfig.h
	${CMAKE_SOURCE_DIR}/third_party/imgui/imgui-knobs.cpp
	${CMAKE_SOURCE_DIR}/third_party/imgui/imgui-knobs.h
	${CMAKE_SOURCE_DIR}/third_party/imgui/imgui.cpp
	${CMAKE_SOURCE_DIR}/third_party/imgui/imgui.h
	${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_demo.cpp
	${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_draw.cpp
	${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_impl_dx9.cpp
	${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_impl_dx9.h
	${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_impl_win32.cpp
	${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_impl_win32.h
	${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_internal.h
	${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_tables.cpp
	${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_widgets.cpp
	${CMAKE_SOURCE_DIR}/third_party/imgui/imstb_rectpack.h
	${CMAKE_SOURCE_DIR}/third_party/imgui/imstb_textedit.h
	${CMAKE_SOURCE_DIR}/third_party/imgui/imstb_truetype.h
)

target_include_directories(WaveSabreVstLib PUBLIC
	include
	../WaveSabreCore/include
	${CMAKE_SOURCE_DIR}/third_party/imgui
	${CMAKE_SOURCE_DIR}/third_party/7z
	${VSTSDK3_DIR}/public.sdk/source/vst2.x
	${VSTSDK3_DIR})

target_compile_features(WaveSabreVstLib PUBLIC cxx_std_17)

if(MSVC)
	target_compile_options(WaveSabreVstLib PUBLIC /EHsc)
	target_link_libraries(WaveSabreVstLib PUBLIC d3d9)
endif()
