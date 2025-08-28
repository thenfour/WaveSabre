file(GLOB_RECURSE MANUAL_TESTS_HEADERS
    "include/*.h"
    "include/*.hpp"
)
file(GLOB_RECURSE MANUAL_TESTS_SOURCES
    "src/*.cpp"
)

add_executable(WaveSabreManualTests WIN32
    ${MANUAL_TESTS_HEADERS}
    ${MANUAL_TESTS_SOURCES}
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_impl_win32.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_impl_dx9.cpp
    ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui-knobs.cpp
)

target_include_directories(WaveSabreManualTests PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/WaveSabreCore/include
    ${CMAKE_SOURCE_DIR}/WaveSabreVstLib/include
    ${CMAKE_SOURCE_DIR}/third_party/imgui
)

target_link_libraries(WaveSabreManualTests PRIVATE
    WaveSabreCore
    WaveSabreVstLib
    d3d9
)

target_compile_features(WaveSabreManualTests PRIVATE cxx_std_20)

if (MSVC)
    target_compile_options(WaveSabreManualTests PRIVATE /EHsc)
endif()

set_target_properties(WaveSabreManualTests PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
