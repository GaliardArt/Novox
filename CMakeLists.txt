cmake_minimum_required(VERSION 3.22)
project(NOVOX VERSION 0.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(NOVOX
    PRODUCT_NAME "NOVOX"
    COMPANY_NAME "NOVOX Project"
)

target_sources(NOVOX PRIVATE
    Source/Main.cpp
    Source/MainComponent.h
    Source/MainComponent.cpp
    Source/PitchDetector.h
    Source/PitchDetector.cpp
    Source/VoiceEngine.h
    Source/VoiceEngine.cpp
    Source/MidiRouter.h
    Source/MidiRouter.cpp
)

target_compile_definitions(NOVOX PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:NOVOX,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
)

target_link_libraries(NOVOX PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

if(BUILD_TESTING)
    add_executable(NOVOXPitchTests
        Tests/PitchDetectorTests.cpp
        Source/PitchDetector.cpp
    )
    target_include_directories(NOVOXPitchTests PRIVATE Source)
    if(MSVC)
        target_compile_options(NOVOXPitchTests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(NOVOXPitchTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME NOVOX.PitchDetector COMMAND NOVOXPitchTests)
endif()
