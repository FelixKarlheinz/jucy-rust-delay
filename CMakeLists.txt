cmake_minimum_required(VERSION 3.22)

project(JUCY_RUST VERSION 0.0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(RUST_DELAY_CRATE_NAME "rust_delay")

add_subdirectory(${RUST_DELAY_CRATE_NAME})

include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(juce)

juce_add_plugin(JucyRust
    COMPANY_NAME "Felix Weiß"
    BUNDLE_ID "com.felixweiss.jucyrust"
    PLUGIN_MANUFACTURER_CODE Fwss
    PLUGIN_CODE JrDp
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "JucyRustDelay"
)

target_sources(JucyRust
    PRIVATE
        PluginProcessor.cpp
        PluginEditor.cpp
)

target_compile_definitions(JucyRust
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(JucyRust
    PRIVATE
        juce::juce_audio_utils
        ${RUST_DELAY_CRATE_NAME}
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_include_directories(JucyRust PRIVATE ${CMAKE_BINARY_DIR}/${RUST_DELAY_CRATE_NAME})
