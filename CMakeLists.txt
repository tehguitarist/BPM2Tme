cmake_minimum_required(VERSION 3.15)

project(BPM2Time VERSION 1.2.0)   # bump VERSION to force Logic to rescan the AU after changes

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
    # Matches the previous Projucer/Xcode build: Apple Silicon only.
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architectures to build")
endif()

enable_testing()

add_subdirectory(libs/JUCE)

# AU only exists on Apple platforms; Windows/Linux ship VST3 only.
if (APPLE)
    set(BPM2TIME_FORMATS AU VST3)
else()
    set(BPM2TIME_FORMATS VST3)
endif()

juce_add_plugin(BPM2Time
    COMPANY_NAME "Leigh Pierce"
    COMPANY_WEBSITE "www.LeighPierce.com"
    BUNDLE_ID com.LeighPierce.BPM2Time
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE I7r3                  # unchanged from the original .jucer — keeps existing installs
                                       # recognised as the same plugin by hosts/DAW sessions
    FORMATS ${BPM2TIME_FORMATS}
    PRODUCT_NAME "BPM2Time"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx
    COPY_PLUGIN_AFTER_BUILD TRUE      # installs to the user plugin folders on build
)

juce_generate_juce_header(BPM2Time)

target_sources(BPM2Time
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(BPM2Time
    PUBLIC
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(BPM2Time
    PRIVATE
        juce::juce_audio_processors
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
