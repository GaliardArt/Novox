cmake_minimum_required(VERSION 3.22)
project(NOVOX VERSION 1.0.0 LANGUAGES CXX)
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
    Source/PitchDetector.h Source/PitchDetector.cpp
    Source/VoiceEngine.h Source/VoiceEngine.cpp
    Source/MidiRouter.h Source/MidiRouter.cpp Source/MidiEventQueue.h
    Source/NoteTracker.h Source/NoteTracker.cpp Source/PitchBend.h Source/PitchSmoother.h
    Source/VocalPercussionDetector.h Source/VocalPercussionDetector.cpp Source/DrumMapping.h
    Source/PresetData.h Source/PresetManager.h Source/PresetManager.cpp
)
target_compile_definitions(NOVOX PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:NOVOX,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
)
target_link_libraries(NOVOX PRIVATE
    juce::juce_audio_utils juce::juce_audio_devices juce::juce_audio_basics
    juce::juce_gui_extra juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags
)
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

    add_executable(NOVOXDrumMappingTests Tests/DrumMappingTests.cpp)
    target_include_directories(NOVOXDrumMappingTests PRIVATE Source)
    add_test(NAME NOVOX.DrumMapping COMMAND NOVOXDrumMappingTests)

    add_executable(NOVOXVocalPercussionTests Tests/VocalPercussionDetectorTests.cpp Source/VocalPercussionDetector.cpp)
    target_include_directories(NOVOXVocalPercussionTests PRIVATE Source)
    add_test(NAME NOVOX.VocalPercussionDetector COMMAND NOVOXVocalPercussionTests)

    add_executable(NOVOXPresetManagerTests Tests/PresetManagerTests.cpp Source/PresetManager.cpp)
    target_include_directories(NOVOXPresetManagerTests PRIVATE Source)
    target_link_libraries(NOVOXPresetManagerTests PRIVATE juce::juce_core)
    add_test(NAME NOVOX.PresetManager COMMAND NOVOXPresetManagerTests)

    if(MSVC)
        target_compile_options(NOVOXPitchTests PRIVATE /W4 /permissive-)
        target_compile_options(NOVOXTrackingTests PRIVATE /W4 /permissive-)
        target_compile_options(NOVOXMidiQueueTests PRIVATE /W4 /permissive-)
        target_compile_options(NOVOXPitchBendTests PRIVATE /W4 /permissive-)
        target_compile_options(NOVOXPitchSmootherTests PRIVATE /W4 /permissive-)
        target_compile_options(NOVOXVocalPercussionTests PRIVATE /W4 /permissive-)
        target_compile_options(NOVOXDrumMappingTests PRIVATE /W4 /permissive-)
        target_compile_options(NOVOXPresetManagerTests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(NOVOXPitchTests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(NOVOXTrackingTests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(NOVOXMidiQueueTests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(NOVOXPitchBendTests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(NOVOXPitchSmootherTests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(NOVOXVocalPercussionTests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(NOVOXDrumMappingTests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(NOVOXPresetManagerTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

# Installable Windows release layout (executable + end-user documentation).
install(TARGETS NOVOX
    RUNTIME DESTINATION .
    BUNDLE DESTINATION .
)
install(FILES
    README.md
    docs/QUICKSTART.md
    docs/RELEASE_NOTES_1.0.0.md
    DESTINATION docs
)

# A portable ZIP is the canonical release artifact. The Inno Setup script
# provides an optional conventional installer for maintainers with ISCC installed.
set(CPACK_PACKAGE_NAME "NOVOX")
set(CPACK_PACKAGE_VENDOR "NOVOX Project")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Offline-first voice-to-MIDI desktop instrument")
set(CPACK_PACKAGE_FILE_NAME "NOVOX-${PROJECT_VERSION}-Windows-x64")
set(CPACK_GENERATOR "ZIP")
include(CPack)
