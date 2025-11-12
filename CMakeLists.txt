cmake_minimum_required(VERSION 3.15)
project(WP_Guardian)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
cmake_policy(SET CMP0148 NEW)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

# vcpkg toolchain
set(CMAKE_TOOLCHAIN_FILE "E:/Documents/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

# 外部ライブラリ
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenCV REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(cpr REQUIRED)
find_package(asio CONFIG REQUIRED)

# ディレクトリ指定
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(IMGUI_DIR ${LIB_DIR}/imgui)
set(WEBSOCKETPP_DIR ${LIB_DIR}/websocketpp)
set(JSON_DIR ${LIB_DIR}/json)
set(IMWS_DIR ${LIB_DIR}/ImWS)
set(BASE64_DIR ${LIB_DIR}/Base64)

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


target_compile_definitions(ImWS PUBLIC ASIO_STANDALONE)

# 実行ファイル
add_executable(WP_Guardian
    main.cpp
    ${IMGUI_SRC}
    app.rc
)

if(MSVC)
    target_link_options(WP_Guardian PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
endif()

target_include_directories(WP_Guardian PUBLIC
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
    ImWS
    Base64
)
add_dependencies(WP_Guardian Base64)

# ビルド後に assets フォルダをコピー
add_custom_command(TARGET WP_Guardian POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:WP_Guardian>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:WP_Guardian>/assets"
)
