cmake_minimum_required(VERSION 3.15)
project(WP_Guardian)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
cmake_policy(SET CMP0148 NEW)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

# 外部ライブラリ
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenCV REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(cpr REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# ディレクトリ指定
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(IMGUI_DIR ${LIB_DIR}/imgui)
set(WEBSOCKETPP_DIR ${LIB_DIR}/websocketpp)
set(JSON_DIR ${LIB_DIR}/json)
# src を include ルートにする
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(IMWS_DIR ${SRC_DIR}/ImWS)
set(WSVARIABLE_DIR ${SRC_DIR}/WSVariable)
set(BASE64_DIR ${SRC_DIR}/Base64)

# ImGui ソース
set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)


# ImWS ライブラリ
add_library(ImWS
    ${IMWS_DIR}/ImWS.cpp
    ${IMWS_DIR}/ImWS.h
)

add_library(Base64 STATIC
    ${BASE64_DIR}/Base64.cpp
    ${BASE64_DIR}/Base64.h
)

add_library(WSVariable
    ${WSVARIABLE_DIR}/WSVariable.cpp
    ${WSVARIABLE_DIR}/WSVariable.h
)

target_include_directories(WSVariable PUBLIC
    ${WSVARIABLE_DIR}
    ${WEBSOCKETPP_DIR}
    $<TARGET_PROPERTY:asio::asio,INTERFACE_INCLUDE_DIRECTORIES>
)

target_include_directories(Base64 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/Base64
)


target_include_directories(ImWS PUBLIC
    ${IMWS_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${WEBSOCKETPP_DIR}
    $<TARGET_PROPERTY:asio::asio,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(ImWS PUBLIC ASIO_STANDALONE)
target_link_libraries(ImWS PRIVATE asio::asio)

target_compile_definitions(WSVariable PUBLIC ASIO_STANDALONE)
target_link_libraries(WSVariable PRIVATE asio::asio)

add_library(ImageViewer STATIC
    ${SRC_DIR}/ImageViewer/ImageViewer.cpp
    ${SRC_DIR}/ImageViewer/ImageViewer.h
    ${SRC_DIR}/ImageViewer/UVUtil.h
)
target_include_directories(ImageViewer PRIVATE
    ${IMGUI_DIR}
)
target_link_libraries(ImageViewer PRIVATE
    glfw
)

# ===== Internal Modules =====
add_library(Utils INTERFACE
    ${SRC_DIR}/Utils/DiffPattern.h
    ${SRC_DIR}/Utils/PathUtil.h
)
target_include_directories(Utils INTERFACE ${SRC_DIR}/Utils)

add_library(ImageProcessing STATIC
    ${SRC_DIR}/ImageProcessing/ImageProcessing.cpp
    ${SRC_DIR}/ImageProcessing/ImageProcessing.h
)
target_link_libraries(ImageProcessing PRIVATE
    ${OpenCV_LIBS}
    cpr::cpr
    Utils
)
add_library(ModuleManager STATIC
    ${SRC_DIR}/ModuleManager/ModuleManager.cpp
    ${SRC_DIR}/ModuleManager/ModuleManager.h
)
target_link_libraries(ModuleManager PRIVATE
    nlohmann_json::nlohmann_json
    Utils
)
target_include_directories(ModuleManager PRIVATE
    ${IMGUI_DIR}
    ${SRC_DIR}/Utils
)

add_library(Server STATIC
    ${SRC_DIR}/Server/Server.cpp
    ${SRC_DIR}/Server/Server.h
)
target_include_directories(Server PUBLIC
    ${WEBSOCKETPP_DIR}
    $<TARGET_PROPERTY:asio::asio,INTERFACE_INCLUDE_DIRECTORIES>
)
add_library(VersionManager STATIC
    ${SRC_DIR}/VersionManager/VersionManager.cpp
    ${SRC_DIR}/VersionManager/VersionManager.h
)
target_compile_definitions(Server PUBLIC ASIO_STANDALONE)
target_link_libraries(Server PRIVATE asio::asio)

target_link_libraries(VersionManager PRIVATE
    cpr::cpr
    nlohmann_json::nlohmann_json
)

set(SETTINGS_CORE_DIR ${SRC_DIR}/Settings/Core)
set(SETTINGS_UI_DIR   ${SRC_DIR}/Settings/UI)

add_library(SettingsCore STATIC
    ${SETTINGS_CORE_DIR}/Settings.h
    ${SETTINGS_CORE_DIR}/SettingsIO.cpp
    ${SETTINGS_CORE_DIR}/SettingsIO.h
    ${SETTINGS_CORE_DIR}/SettingsJSON.cpp
    ${SETTINGS_CORE_DIR}/SettingsJSON.h
)
target_include_directories(SettingsCore PUBLIC
    ${SETTINGS_CORE_DIR}
    ${SRC_DIR}/Utils
)
target_link_libraries(SettingsCore PRIVATE
    Utils
    nlohmann_json::nlohmann_json
)
add_library(SettingsUI STATIC
    ${SETTINGS_UI_DIR}/AppSettingsWindow.cpp
    ${SETTINGS_UI_DIR}/AppSettingsWindow.h
    ${SETTINGS_UI_DIR}/ModuleSettingsWindow.cpp
    ${SETTINGS_UI_DIR}/ModuleSettingsWindow.h
    ${SETTINGS_UI_DIR}/UISettingsWindow.cpp
    ${SETTINGS_UI_DIR}/UISettingsWindow.h
)
target_include_directories(SettingsUI PUBLIC
    ${SETTINGS_UI_DIR}
    ${IMGUI_DIR}
)
target_link_libraries(SettingsUI PRIVATE
    SettingsCore
    glfw
)
add_library(Base64Codec STATIC
    ${SRC_DIR}/Base64Codec/Base64Codec.cpp
    ${SRC_DIR}/Base64Codec/Base64Codec.h
)

target_include_directories(Base64Codec PRIVATE
    ${SRC_DIR}/Base64
    ${LIB_DIR}/base64-SIMD-C/include
)

target_link_libraries(Base64Codec PRIVATE
    Base64
    ${LIB_DIR}/base64-SIMD-C/lib/base64-SIMD-C.lib
)

target_compile_definitions(Base64Codec PRIVATE USE_DLL)

# 実行ファイル
add_executable(WP_Guardian
    ${SRC_DIR}/main.cpp
    ${IMGUI_SRC}
    app.rc
)


if(MSVC)
    target_link_options(WP_Guardian PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
    add_compile_options(/utf-8)
endif()

target_include_directories(WP_Guardian PRIVATE
    ${SRC_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(WP_Guardian PRIVATE
    glfw
    OpenGL::GL
    ${OpenCV_LIBS}
    GLEW::GLEW
    cpr::cpr
    # internal modules
    ImageProcessing
    ModuleManager
    Server
    VersionManager
    ImWS
    Base64Codec
    WSVariable
    ImageViewer
    SettingsCore
    SettingsUI
    Utils
)

# ビルド後に assets フォルダをコピー
add_custom_command(TARGET WP_Guardian POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:WP_Guardian>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:WP_Guardian>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIB_DIR}/base64-SIMD-C/bin/base64-SIMD-C.dll"
        "$<TARGET_FILE_DIR:WP_Guardian>/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/imgui.ini"
        "$<TARGET_FILE_DIR:WP_Guardian>/"
    VERBATIM
)