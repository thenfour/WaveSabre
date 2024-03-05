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
	7z/7zTypes.h
	7z/Compiler.h
	7z/CpuArch.c
	7z/CpuArch.h
	7z/LzFind.c
	7z/LzFind.h
	7z/LzFindMt.c
	7z/LzFindMt.h
	7z/LzFindOpt.c
	7z/LzHash.h
	7z/LzmaEnc.c
	7z/LzmaEnc.h
	7z/Precomp.h
	7z/Threads.c
	7z/Threads.h
)

target_include_directories(WaveSabreVstLib PUBLIC
	include
	../WaveSabreCore/include
	${VSTSDK3_DIR}/public.sdk/source/vst2.x
	${VSTSDK3_DIR})

target_compile_features(WaveSabreVstLib PUBLIC cxx_std_17)

if(MSVC)
	target_compile_options(WaveSabreVstLib PUBLIC /EHsc)
endif()
