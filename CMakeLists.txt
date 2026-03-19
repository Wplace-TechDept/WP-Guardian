cmake_minimum_required(VERSION 3.15)
project(WP_Guardian)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
cmake_policy(SET CMP0148 NEW)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

option(ENABLE_MODULE_SYSTEM "Enable WebSocket module system" ON)

# ディレクトリ指定
set(LIB_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(IMGUI_DIR   ${LIB_DIR}/imgui)
set(WEBSOCKETPP_DIR ${LIB_DIR}/websocketpp)
set(SRC_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(MODULE_DIR  ${SRC_DIR}/Module)


# 外部ライブラリ
if(NOT EMSCRIPTEN)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(cpr REQUIRED)
    find_package(asio CONFIG REQUIRED)
endif()
find_package(OpenCV REQUIRED)
if(EMSCRIPTEN)
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE ${LIB_DIR})
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
else()
    find_package(nlohmann_json CONFIG REQUIRED)
endif()

# ImGui ソース
set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# ===== Utils =====
add_library(Utils INTERFACE
    ${SRC_DIR}/Utils/DiffPattern.h
    ${SRC_DIR}/Utils/PathUtil.h
)
target_include_directories(Utils INTERFACE ${SRC_DIR}/Utils)

# ===== ImageViewer =====
add_library(ImageViewer STATIC
    ${SRC_DIR}/ImageViewer/ImageViewer.cpp
    ${SRC_DIR}/ImageViewer/ImageViewer.h
    ${SRC_DIR}/ImageViewer/UVUtil.h
)
target_include_directories(ImageViewer PRIVATE
    ${IMGUI_DIR}
)
if(NOT EMSCRIPTEN)
    target_link_libraries(ImageViewer PRIVATE glfw)
endif()

# ===== ImageFetcher =====
add_library(ImageFetcher STATIC
    ${SRC_DIR}/ImageProcessing/ImageFetcher/ImageFetcher.cpp
    ${SRC_DIR}/ImageProcessing/ImageFetcher/ImageFetcher.h
)
target_link_libraries(ImageFetcher PRIVATE
    ${OpenCV_LIBS}
)
if(NOT EMSCRIPTEN)
    target_link_libraries(ImageFetcher PRIVATE cpr::cpr)
endif()

# ===== ImageProcessing =====
add_library(ImageProcessing STATIC
    ${SRC_DIR}/ImageProcessing/ImageProcessing.cpp
    ${SRC_DIR}/ImageProcessing/ImageProcessing.h
)
target_link_libraries(ImageProcessing PRIVATE
    ${OpenCV_LIBS}
    ImageFetcher
    Utils
)

# ===== VersionManager =====
add_library(VersionManager STATIC
    ${SRC_DIR}/VersionManager/VersionManager.cpp
    ${SRC_DIR}/VersionManager/VersionManager.h
)
target_link_libraries(VersionManager PRIVATE
    nlohmann_json::nlohmann_json
)
if(NOT EMSCRIPTEN)
    target_link_libraries(VersionManager PRIVATE cpr::cpr)
endif()

# ===== Settings =====
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
    ${SETTINGS_UI_DIR}/UISettingsWindow.cpp
    ${SETTINGS_UI_DIR}/UISettingsWindow.h
)
target_include_directories(SettingsUI PUBLIC
    ${SETTINGS_UI_DIR}
    ${IMGUI_DIR}
)
target_link_libraries(SettingsUI PRIVATE
    SettingsCore
)
if(NOT EMSCRIPTEN)
    target_link_libraries(SettingsUI PRIVATE glfw)
endif()

# ===== Module =====
set(MODULE_SETTINGS_CORE_DIR ${MODULE_DIR}/ModuleSettings/Core)
set(MODULE_SETTINGS_UI_DIR   ${MODULE_DIR}/ModuleSettings/UI)

add_library(ModuleSettingsCore STATIC
    ${MODULE_SETTINGS_CORE_DIR}/ModuleSettings.h
    ${MODULE_SETTINGS_CORE_DIR}/ModuleSettingsIO.cpp
    ${MODULE_SETTINGS_CORE_DIR}/ModuleSettingsIO.h
    ${MODULE_SETTINGS_CORE_DIR}/ModuleSettingsJSON.cpp
    ${MODULE_SETTINGS_CORE_DIR}/ModuleSettingsJSON.h
)
target_include_directories(ModuleSettingsCore PUBLIC
    ${MODULE_SETTINGS_CORE_DIR}
    ${SRC_DIR}
)
target_link_libraries(ModuleSettingsCore PRIVATE
    nlohmann_json::nlohmann_json
    Utils
)

if(ENABLE_MODULE_SYSTEM)

    add_library(Base64 STATIC
        ${MODULE_DIR}/base64/base64.cpp
        ${MODULE_DIR}/base64/base64.h
    )
    target_include_directories(Base64 PUBLIC
        ${MODULE_DIR}/base64
    )

    add_library(Base64Codec STATIC
        ${MODULE_DIR}/Base64Codec/Base64Codec.cpp
        ${MODULE_DIR}/Base64Codec/Base64Codec.h
    )
    target_include_directories(Base64Codec PRIVATE
        ${MODULE_DIR}/base64
        ${LIB_DIR}/base64-SIMD-C/include
    )
    target_link_libraries(Base64Codec PRIVATE
        Base64
        ${LIB_DIR}/base64-SIMD-C/lib/base64-SIMD-C.lib
    )
    target_compile_definitions(Base64Codec PRIVATE USE_DLL)

    add_library(ImWS STATIC
        ${MODULE_DIR}/ImWS/ImWS.cpp
        ${MODULE_DIR}/ImWS/ImWS.h
    )
    target_include_directories(ImWS PUBLIC
        ${MODULE_DIR}/ImWS
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${WEBSOCKETPP_DIR}
        $<TARGET_PROPERTY:asio::asio,INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(ImWS PUBLIC ASIO_STANDALONE)
    target_link_libraries(ImWS PRIVATE asio::asio)

    add_library(WSVariable STATIC
        ${MODULE_DIR}/WSVariable/WSVariable.cpp
        ${MODULE_DIR}/WSVariable/WSVariable.h
    )
    target_include_directories(WSVariable PUBLIC
        ${MODULE_DIR}/WSVariable
        ${WEBSOCKETPP_DIR}
        $<TARGET_PROPERTY:asio::asio,INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(WSVariable PUBLIC ASIO_STANDALONE)
    target_link_libraries(WSVariable PRIVATE asio::asio)

    add_library(Server STATIC
        ${MODULE_DIR}/Server/Server.cpp
        ${MODULE_DIR}/Server/Server.h
    )
    target_include_directories(Server PUBLIC
        ${MODULE_DIR}/Server
        ${WEBSOCKETPP_DIR}
        $<TARGET_PROPERTY:asio::asio,INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(Server PUBLIC ASIO_STANDALONE)
    target_link_libraries(Server PRIVATE asio::asio)

    add_library(ModuleManager STATIC
        ${MODULE_DIR}/ModuleManager/ModuleManager.cpp
        ${MODULE_DIR}/ModuleManager/ModuleManager.h
    )
    target_include_directories(ModuleManager PRIVATE
        ${IMGUI_DIR}
        ${SRC_DIR}/Utils
    )
    target_link_libraries(ModuleManager PRIVATE
        nlohmann_json::nlohmann_json
        Utils
    )

    add_library(ModuleSettingsUI STATIC
        ${MODULE_SETTINGS_UI_DIR}/ModuleSettingsWindow.cpp
        ${MODULE_SETTINGS_UI_DIR}/ModuleSettingsWindow.h
    )
    target_include_directories(ModuleSettingsUI PUBLIC
        ${MODULE_SETTINGS_UI_DIR}
        ${IMGUI_DIR}
    )
    target_link_libraries(ModuleSettingsUI PRIVATE
        ModuleSettingsCore
    )

    add_library(ModuleSystemLib STATIC
        ${MODULE_DIR}/ModuleSystem.cpp
        ${MODULE_DIR}/ModuleSystem.h
    )
    target_include_directories(ModuleSystemLib PUBLIC
        ${MODULE_DIR}
        ${SRC_DIR}
        ${IMGUI_DIR}
        ${WEBSOCKETPP_DIR}
        ${OpenCV_INCLUDE_DIRS}
        $<TARGET_PROPERTY:asio::asio,INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(ModuleSystemLib PUBLIC
        ASIO_STANDALONE
        ENABLE_MODULE_SYSTEM
    )
    target_link_libraries(ModuleSystemLib PRIVATE
        asio::asio
        nlohmann_json::nlohmann_json
        ImWS
        WSVariable
        Server
        ModuleManager
        Base64Codec
        ModuleSettingsCore
        ModuleSettingsUI
        SettingsCore
        ${OpenCV_LIBS}
    )

endif()

# ===== 実行ファイル =====
if(EMSCRIPTEN)
    add_executable(WP_Guardian
        ${SRC_DIR}/main.cpp
        ${IMGUI_SRC}
    )
else()
    add_executable(WP_Guardian
        ${SRC_DIR}/main.cpp
        ${IMGUI_SRC}
        app.rc
    )
endif()

if(MSVC)
    target_link_options(WP_Guardian PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
    add_compile_options(/utf-8)
endif()

if(EMSCRIPTEN)
    set_target_properties(WP_Guardian PROPERTIES
        SUFFIX ".html"
    )
    target_link_options(WP_Guardian PRIVATE
        -sUSE_GLFW=3
        -sUSE_WEBGL2=1
        -sFULL_ES3=1
        -sALLOW_MEMORY_GROWTH=1
        -sASYNCIFY=1
        --preload-file assets
    )
endif()

target_include_directories(WP_Guardian PRIVATE
    ${SRC_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${OpenCV_INCLUDE_DIRS}
)
if(EMSCRIPTEN)
    target_link_libraries(WP_Guardian PRIVATE
        ${OpenCV_LIBS}
        ImageProcessing
        VersionManager
        ImageViewer
        SettingsCore
        SettingsUI
        ModuleSettingsCore
        Utils
        # glfw, OpenGL, GLEWはEmscriptenが内包
    )
else()
    target_link_libraries(WP_Guardian PRIVATE
        glfw
        OpenGL::GL
        ${OpenCV_LIBS}
        GLEW::GLEW
        cpr::cpr
        ImageProcessing
        VersionManager
        ImageViewer
        SettingsCore
        SettingsUI
        ModuleSettingsCore
        Utils
        $<$<BOOL:${ENABLE_MODULE_SYSTEM}>:ModuleSystemLib>
    )
endif()
# ===== ビルド後処理 =====
add_custom_command(TARGET WP_Guardian POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:WP_Guardian>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:WP_Guardian>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/imgui.ini"
        "$<TARGET_FILE_DIR:WP_Guardian>/"
    VERBATIM
)

if(ENABLE_MODULE_SYSTEM)
    add_custom_command(TARGET WP_Guardian POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${LIB_DIR}/base64-SIMD-C/bin/base64-SIMD-C.dll"
            "$<TARGET_FILE_DIR:WP_Guardian>/"
        VERBATIM
    )
endif()