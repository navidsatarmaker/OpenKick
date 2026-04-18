cmake_minimum_required(VERSION 3.20)

project(OpenKick VERSION 1.0.0)

# Add JUCE via FetchContent
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(juce)

# Create the Audio Plugin target
juce_add_plugin(OpenKick
    COMPANY_NAME "OpenKick contributors"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    PLUGIN_MANUFACTURER_CODE OpKk
    PLUGIN_CODE OKck
    FORMATS VST3 Standalone
    PRODUCT_NAME "OpenKick")

# Source files
target_sources(OpenKick
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)

juce_generate_juce_header(OpenKick)

# Definitions
target_compile_definitions(OpenKick
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

# Link JUCE modules
target_link_libraries(OpenKick
    PRIVATE
        juce::juce_audio_utils
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# C++ Standard
target_compile_features(OpenKick PUBLIC cxx_std_20)
