cmake_minimum_required(VERSION 3.20)
project(Nebula VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- VST3 SDK -----------------------------------------------------------------
set(VST3_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/vst3sdk" CACHE PATH "Path to VST3 SDK")
if(NOT EXISTS "${VST3_SDK_PATH}/pluginterfaces/vst/ivstaudioprocessor.h")
    message(FATAL_ERROR "VST3 SDK not found at ${VST3_SDK_PATH}. Run: git clone --recursive https://github.com/steinbergmedia/vst3sdk.git external/vst3sdk")
endif()

set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)
set(SMTG_RUN_VST_VALIDATOR  OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${VST3_SDK_PATH}" "${CMAKE_BINARY_DIR}/vst3sdk")

# --- WebView2 (via NuGet) -----------------------------------------------------
# We'll vendored-import via .targets. For brevity, expect WebView2 headers/libs in:
#   external/webview2/build/native/include
#   external/webview2/build/native/x64/WebView2Loader.dll.lib
set(WEBVIEW2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/webview2" CACHE PATH "Path to WebView2 SDK")

# --- Plugin -------------------------------------------------------------------
set(target Nebula)

set(SRC
    src/plugin/NebulaFactory.cpp
    src/plugin/NebulaProcessor.cpp
    src/plugin/NebulaController.cpp
    src/plugin/NebulaEditor.cpp
    src/dsp/Voice.cpp
)

set(HDR
    src/plugin/NebulaProcessor.h
    src/plugin/NebulaController.h
    src/plugin/NebulaEditor.h
    src/plugin/ParameterIds.h
    src/dsp/Voice.h
    src/dsp/PolyBlepOsc.h
    src/dsp/SvfFilter.h
    src/dsp/Envelope.h
    src/dsp/Lfo.h
    src/dsp/Chorus.h
    src/dsp/Delay.h
    src/dsp/Reverb.h
)

# resource: embed synth.html as a string
set(HTML_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/synth.html")
set(HTML_OUT "${CMAKE_CURRENT_BINARY_DIR}/EmbeddedHtml.h")
add_custom_command(
    OUTPUT  ${HTML_OUT}
    COMMAND ${CMAKE_COMMAND} -DINPUT_FILE=${HTML_SRC} -DOUTPUT_FILE=${HTML_OUT} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedFile.cmake
    DEPENDS ${HTML_SRC}
)
list(APPEND HDR ${HTML_OUT})

smtg_add_vst3plugin(${target} ${SRC} ${HDR})

target_include_directories(${target} PRIVATE
    src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${WEBVIEW2_DIR}/build/native/include
)

target_link_libraries(${target} PRIVATE
    sdk
    base
    pluginterfaces
    ${WEBVIEW2_DIR}/build/native/x64/WebView2LoaderStatic.lib
    user32.lib ole32.lib oleaut32.lib shlwapi.lib version.lib advapi32.lib gdi32.lib
)

target_compile_definitions(${target} PRIVATE
    UNICODE _UNICODE
    NOMINMAX
)

if(MSVC)
    target_compile_options(${target} PRIVATE /W3 /MP /utf-8)
endif()

smtg_target_configure_version_file(${target})
