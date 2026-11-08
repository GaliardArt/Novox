cmake_minimum_required(VERSION 3.22)
project(NOVOX VERSION 1.2.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CTest)
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(NOVOX PRODUCT_NAME "NOVOX" COMPANY_NAME "NOVOX Project")
juce_generate_juce_header(NOVOX)
target_sources(NOVOX PRIVATE
    Source/Main.cpp Source/MainComponent.h Source/MainComponent.cpp
    Source/NovoxLookAndFeel.h Source/NovoxLookAndFeel.cpp
    Source/PitchDetector.h Source/PitchDetector.cpp Source/NoiseReducer.h
    Source/VoiceEngine.h Source/VoiceEngine.cpp Source/InputChannelSelector.h
    Source/MidiRouter.h Source/MidiRouter.cpp Source/MidiEventQueue.h
    Source/NoteTracker.h Source/NoteTracker.cpp Source/PitchBend.h Source/PitchSmoother.h
    Source/PresetData.h Source/PresetManager.h Source/PresetManager.cpp
)
target_compile_definitions(NOVOX PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:NOVOX,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
)
target_link_libraries(NOVOX PRIVATE
    juce::juce_audio_utils juce::juce_audio_devices juce::juce_audio_basics juce::juce_dsp
    juce::juce_recommended_config_flags juce::juce_recommended_warning_flags
)
if(WIN32)
    target_compile_options(NOVOX PRIVATE
        "$<$<CONFIG:Debug>:/Od>"
        "$<$<CONFIG:Release>:/O2>"
        /MP /EHsc /utf-8
    )
endif()

if(BUILD_TESTING)
    add_executable(NOVOXPitchTests Tests/PitchDetectorTests.cpp Source/PitchDetector.cpp)
    target_include_directories(NOVOXPitchTests PRIVATE Source)
    add_test(NAME NOVOX.PitchDetector COMMAND NOVOXPitchTests)
    add_executable(NOVOXTrackingTests Tests/NoteTrackerTests.cpp Source/NoteTracker.cpp)
    target_include_directories(NOVOXTrackingTests PRIVATE Source)
    add_test(NAME NOVOX.NoteTracker COMMAND NOVOXTrackingTests)
    add_executable(NOVOXMidiQueueTests Tests/MidiEventQueueTests.cpp)
    target_include_directories(NOVOXMidiQueueTests PRIVATE Source)
    add_test(NAME NOVOX.MidiEventQueue COMMAND NOVOXMidiQueueTests)
    add_executable(NOVOXPitchBendTests Tests/PitchBendTests.cpp)
    target_include_directories(NOVOXPitchBendTests PRIVATE Source)
    add_test(NAME NOVOX.PitchBend COMMAND NOVOXPitchBendTests)
    add_executable(NOVOXPitchSmootherTests Tests/PitchSmootherTests.cpp)
    target_include_directories(NOVOXPitchSmootherTests PRIVATE Source)
    add_test(NAME NOVOX.PitchSmoother COMMAND NOVOXPitchSmootherTests)
    add_executable(NOVOXPresetManagerTests Tests/PresetManagerTests.cpp Source/PresetManager.cpp)
    target_include_directories(NOVOXPresetManagerTests PRIVATE Source)
    target_link_libraries(NOVOXPresetManagerTests PRIVATE juce::juce_core)
    add_test(NAME NOVOX.PresetManager COMMAND NOVOXPresetManagerTests)
    add_executable(NOVOXNoiseReducerTests Tests/NoiseReducerTests.cpp)
    target_include_directories(NOVOXNoiseReducerTests PRIVATE Source)
    target_link_libraries(NOVOXNoiseReducerTests PRIVATE juce::juce_dsp)
    add_test(NAME NOVOX.NoiseReducer COMMAND NOVOXNoiseReducerTests)
    add_executable(NOVOXInputChannelTests Tests/InputChannelSelectorTests.cpp)
    target_include_directories(NOVOXInputChannelTests PRIVATE Source)
    add_test(NAME NOVOX.InputChannelSelector COMMAND NOVOXInputChannelTests)
    if(MSVC)
        foreach(testTarget NOVOXPitchTests NOVOXTrackingTests NOVOXMidiQueueTests NOVOXPitchBendTests NOVOXPitchSmootherTests NOVOXPresetManagerTests NOVOXNoiseReducerTests NOVOXInputChannelTests)
            target_compile_options(${testTarget} PRIVATE /W4 /permissive-)
        endforeach()
    else()
        foreach(testTarget NOVOXPitchTests NOVOXTrackingTests NOVOXMidiQueueTests NOVOXPitchBendTests NOVOXPitchSmootherTests NOVOXPresetManagerTests NOVOXNoiseReducerTests NOVOXInputChannelTests)
            target_compile_options(${testTarget} PRIVATE -Wall -Wextra -Wpedantic)
        endforeach()
    endif()
endif()

install(TARGETS NOVOX RUNTIME DESTINATION . BUNDLE DESTINATION .)
install(FILES README.md docs/QUICKSTART.md docs/RELEASE_NOTES_1.0.0.md
    docs/RELEASE_NOTES_1.1.0.md docs/RELEASE_NOTES_1.2.0.md DESTINATION docs)
set(CPACK_PACKAGE_NAME "NOVOX")
set(CPACK_PACKAGE_VENDOR "NOVOX Project")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Offline-first voice-to-MIDI desktop instrument")
set(CPACK_PACKAGE_FILE_NAME "NOVOX-${PROJECT_VERSION}-Windows-x64")
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}")
include(CPack)
