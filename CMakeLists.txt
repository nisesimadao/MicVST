cmake_minimum_required(VERSION 3.22)
project(MicVST VERSION 1.4.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.13
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

add_library(MicVSTCore STATIC
    src/audio/Metering.cpp
    src/audio/GraphConnections.cpp
    src/audio/ScanCoordinator.cpp
    src/audio/AudioPadEngine.cpp
    src/state/Persistence.cpp
    src/state/PluginScanCache.cpp
    src/ui/PluginSearch.cpp
)
target_compile_features(MicVSTCore PUBLIC cxx_std_17)
target_include_directories(MicVSTCore PUBLIC src)
# AudioPadEngine advertises these formats in the UI. JUCE always registers WAV/AIFF,
# while MP3/FLAC/Ogg readers are conditional, so enable them explicitly for every
# target that consumes MicVSTCore instead of relying on JUCE/Projucer defaults.
target_compile_definitions(MicVSTCore PUBLIC
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_USE_FLAC=1
    JUCE_USE_OGGVORBIS=1
)
target_link_libraries(MicVSTCore PUBLIC
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_core
    juce::juce_data_structures
)

juce_add_gui_app(MicVST
    PRODUCT_NAME "MicVST"
    COMPANY_NAME "MicVST"
    ICON_BIG   "${CMAKE_CURRENT_SOURCE_DIR}/resources/iconDarkMode.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/resources/iconDarkMode.png"
)

juce_add_binary_data(MicVSTData SOURCES
    resources/iconDarkMode.png
    resources/iconLightMode.png
)

target_sources(MicVST PRIVATE
    src/Main.cpp
    src/audio/AudioEngine.cpp
    src/audio/SecondaryOutput.cpp
    src/audio/PluginChain.cpp
    src/audio/BuiltinEffects.cpp
    src/ui/MainComponent.cpp
    src/ui/DevicePanel.cpp
    src/ui/AudioPadPanel.cpp
    src/ui/GlobalHotkey.cpp
    src/ui/PluginListView.cpp
    src/ui/PluginPicker.cpp
    src/ui/LevelMeterComponent.cpp
    src/ui/TrayIcon.cpp
    src/state/AutostartRegistry.cpp
)
target_compile_definitions(MicVST PRIVATE
    MICVST_VERSION="${PROJECT_VERSION}"
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=0
    JUCE_ASIO=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)
target_link_libraries(MicVST PRIVATE
    MicVSTCore
    MicVSTData
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

if(WIN32)
    add_executable(MicVSTDriverInstaller tools/DriverInstaller.cpp)
    target_compile_features(MicVSTDriverInstaller PRIVATE cxx_std_17)
    target_compile_definitions(MicVSTDriverInstaller PRIVATE UNICODE _UNICODE)
    target_link_libraries(MicVSTDriverInstaller PRIVATE setupapi newdev)
endif()

juce_add_console_app(MicVSTTests PRODUCT_NAME "MicVSTTests")
target_sources(MicVSTTests PRIVATE
    tests/TestMain.cpp
    tests/ScanTests.cpp
    tests/PickerTests.cpp
    tests/BuiltinEffectsTests.cpp
    tests/Output2Tests.cpp
    tests/AudioPadTests.cpp
    src/audio/BuiltinEffects.cpp
)
target_compile_definitions(MicVSTTests PRIVATE
    JUCE_PLUGINHOST_VST3=1
)
target_link_libraries(MicVSTTests PRIVATE
    MicVSTCore
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
