add_library(WaveSabreVstLib STATIC
	${VSTSDK3_DIR}/public.sdk/source/vst2.x/audioeffect.cpp
	${VSTSDK3_DIR}/public.sdk/source/vst2.x/audioeffectx.cpp
	${VSTSDK3_DIR}/public.sdk/source/vst2.x/vstplugmain.cpp
	include/WaveSabreVstLib.h
	include/WaveSabreVstLib/Common.h
	include/WaveSabreVstLib/Serialization.hpp
	include/WaveSabreVstLib/VstEditor.h
	include/WaveSabreVstLib/VstPlug.h
	src/VstEditor.cpp
	src/VstPlug.cpp
	imgui/imconfig.h
	imgui/imgui-knobs.cpp
	imgui/imgui-knobs.h
	imgui/imgui.cpp
	imgui/imgui.h
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_impl_dx9.cpp
	imgui/imgui_impl_dx9.h
	imgui/imgui_impl_win32.cpp
	imgui/imgui_impl_win32.h
	imgui/imgui_internal.h
	imgui/imgui_tables.cpp
	imgui/imgui_widgets.cpp
	imgui/imstb_rectpack.h
	imgui/imstb_textedit.h
	imgui/imstb_truetype.h
)

target_include_directories(WaveSabreVstLib PUBLIC
	include
	../WaveSabreCore/include
	${VSTSDK3_DIR}/public.sdk/source/vst2.x
	${VSTSDK3_DIR})

target_compile_features(WaveSabreVstLib PUBLIC cxx_std_17)

if(MSVC)
	target_compile_definitions(WaveSabreVstLib PUBLIC _CRT_SECURE_NO_WARNINGS)
	target_compile_definitions(WaveSabreVstLib PUBLIC WINVER=0x0A00)
	target_compile_definitions(WaveSabreVstLib PUBLIC _WIN32_WINNT=0x0A00)
	target_compile_options(WaveSabreVstLib PUBLIC /EHsc)
endif()
