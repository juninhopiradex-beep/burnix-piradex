cmake_minimum_required(VERSION 3.22)

# macOS: binario universal (arm64 + x86_64), minimo 10.13 -> corre no MacBook Pro 2013 (Catalina, Intel) e em Macs recentes.
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "macOS minimo")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Arquiteturas")
endif()
# Windows: runtime do Visual C++ embutido (nao precisa do VC++ Redistributable)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(BURNIX VERSION 1.3.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- testes do DSP puro (sem JUCE): nao precisam do JUCE
option(BX_BUILD_TESTS "Compilar os testes DSP" OFF)
if(BX_BUILD_TESTS)
    add_executable(test_dsp Tests/test_dsp.cpp)
    target_include_directories(test_dsp PRIVATE Source)
    enable_testing()
    add_test(NAME dsp COMMAND test_dsp)
endif()
option(BX_ONLY_TESTS "So os testes DSP (sem JUCE)" OFF)
if(BX_ONLY_TESTS)
    return()
endif()

# JUCE 8.0.15 (GPLv3 / licenca comercial JUCE — ver Docs/LICENSES.md)
if(DEFINED JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.15 GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

if(APPLE)
    set(BX_FORMATS VST3 AU Standalone)
else()
    set(BX_FORMATS VST3 Standalone)
endif()

juce_add_plugin(Burnix
    COMPANY_NAME "Piradex"
    BUNDLE_ID "com.piradex.burnix"
    PLUGIN_MANUFACTURER_CODE Prdx
    PLUGIN_CODE PxBx
    FORMATS ${BX_FORMATS}
    PRODUCT_NAME "BURNIX by PIRADEX"
    DESCRIPTION "BURNIX - saturador de valvulas multibanda com Mid/Side"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect
    MICROPHONE_PERMISSION_ENABLED FALSE)

set(BX_SOURCES Source/Plugin/PluginProcessor.cpp Source/Plugin/Access.cpp Source/GUI/PluginEditor.cpp)
target_sources(Burnix PRIVATE ${BX_SOURCES})
target_include_directories(Burnix PRIVATE Source)
target_compile_definitions(Burnix PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_MODAL_LOOPS_PERMITTED=0)

# PCC (Plugin Control Central): mesma infraestrutura e chave publica dos outros plugins Piradex
set(BX_PCC_URL "https://ejjeittwyjttmevhajxo.supabase.co" CACHE STRING "URL do projeto Supabase do PCC")
set(BX_PCC_PUBKEY "064c37512051906b9c8f1a9dfc962e9a1cd4c9f5b0f44ce66a12842b8e70a868" CACHE STRING "Chave publica Ed25519 do PCC (64 hex)")
if(BX_PCC_URL AND BX_PCC_PUBKEY)
    target_compile_definitions(Burnix PUBLIC PQ_PCC_URL="${BX_PCC_URL}" PQ_PCC_PUBKEY="${BX_PCC_PUBKEY}")
endif()
if(MSVC)
    target_compile_options(Burnix PRIVATE /utf-8 /bigobj)
endif()

target_link_libraries(Burnix
    PRIVATE juce::juce_audio_utils juce::juce_audio_processors juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags)
option(BX_ENABLE_LTO "Otimizacao em tempo de ligacao (CI)" ON)
if(BX_ENABLE_LTO)
    target_link_libraries(Burnix PUBLIC juce::juce_recommended_lto_flags)
endif()

# ---- teste de integracao (processador + editor, sem anfitriao): medicoes reais, null, latencia, screenshots
option(BX_BUILD_SMOKE "Compilar o teste de integracao" OFF)
if(BX_BUILD_SMOKE)
    juce_add_console_app(BurnixSmoke PRODUCT_NAME "burnix_smoke")
    target_sources(BurnixSmoke PRIVATE Tests/smoke.cpp ${BX_SOURCES})
    target_include_directories(BurnixSmoke PRIVATE Source)
    target_compile_definitions(BurnixSmoke PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_STANDALONE_APPLICATION=1
        JucePlugin_VersionString="${PROJECT_VERSION}" JucePlugin_Name="BURNIX by PIRADEX" JUCE_PLUGINHOST_AU=0
        PQ_PCC_URL="${BX_PCC_URL}" PQ_PCC_PUBKEY="${BX_PCC_PUBKEY}")
    target_link_libraries(BurnixSmoke PRIVATE juce::juce_audio_utils juce::juce_audio_processors juce::juce_dsp juce::juce_recommended_config_flags)
endif()
