cmake_minimum_required(VERSION 3.20)

project(binaryninja-esp-app C CXX)

if(NOT BN_API_PATH)
    set(BN_API_PATH ${PROJECT_SOURCE_DIR}/submodules/binaryninja-api)
endif()
if(EXISTS ${BN_API_PATH})
    add_subdirectory(${BN_API_PATH} ${PROJECT_BINARY_DIR}/binaryninja-api)
else()
    message(FATAL_ERROR "Binary Ninja API not found at ${BN_API_PATH}")
endif()

add_library(view_esp_app SHARED 
    src/esp_app_plugin.cpp
    src/esp_app_view_type.cpp
    src/esp_app_view.cpp
    src/esp32.cpp
)

if(WIN32)
    get_target_property(BN_API_SOURCE_DIR binaryninjaapi SOURCE_DIR)
    list(APPEND CMAKE_MODULE_PATH "${BN_API_SOURCE_DIR}/cmake")
    find_package(BinaryNinjaCore REQUIRED)
    target_link_libraries(view_esp_app 
        ${BinaryNinjaCore_LIBRARIES}
        binaryninjaapi)
    target_link_directories(view_esp_app PRIVATE ${BinaryNinjaCore_LIBRARY_DIRS})
else()
    target_link_libraries(view_esp_app binaryninjaapi)
endif()

set_target_properties(view_esp_app PROPERTIES
    CXX_STANDARD 20
    CXX_VISIBILITY_PRESET hidden
    CXX_STANDARD_REQUIRED ON
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
)
