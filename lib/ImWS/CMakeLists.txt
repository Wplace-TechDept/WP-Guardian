add_library(ImWS
    ImWS.cpp
    ImWS.h
)

target_include_directories(ImWS PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${WEBSOCKETPP_DIR}
    $<TARGET_PROPERTY:asio::asio,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(ImWS PUBLIC ASIO_STANDALONE)

target_link_libraries(ImWS PRIVATE
    asio::asio
    nlohmann_json::nlohmann_json
)
