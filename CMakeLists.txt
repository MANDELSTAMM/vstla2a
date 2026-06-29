cmake_minimum_required(VERSION 3.22)

project(ALX_LA_2A VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Путь к JUCE (клонируется рядом, либо укажите свой через -DJUCE_DIR=...)
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    message(FATAL_ERROR "JUCE не найден в ${CMAKE_CURRENT_SOURCE_DIR}/JUCE — склонируйте https://github.com/juce-framework/JUCE.git в эту папку")
endif()

add_subdirectory(JUCE)

juce_add_plugin(ALX_LA_2A
    VERSION "1.0.0"
    COMPANY_NAME "ALXAudio"
    PLUGIN_MANUFACTURER_CODE Alxa
    PLUGIN_CODE AlA2
    FORMATS VST3
    PRODUCT_NAME "ALX-LA-2A"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES "Fx" "Dynamics"
)

juce_generate_juce_header(ALX_LA_2A)

target_sources(ALX_LA_2A PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/VUMeterComponent.cpp
)

target_compile_definitions(ALX_LA_2A PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(ALX_LA_2A PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_audio_processors
    juce::juce_audio_basics
    juce::juce_gui_basics
    juce::juce_gui_extra
)
