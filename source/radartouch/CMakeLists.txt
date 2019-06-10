cmake_minimum_required(VERSION 3.0)

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/min-api)

include(${C74_MIN_API_DIR}/script/min-pretarget.cmake)

add_library(
    ${PROJECT_NAME}
    MODULE
    ${PROJECT_NAME}.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC ${C74_INCLUDES})

target_link_libraries(${PROJECT_NAME} PUBLIC radartouch_shared_headers)

include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)

liboh_setup(${PROJECT_NAME})

version_tag(${PROJECT_NAME})

if(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC "/wd4244")
    target_compile_options(${PROJECT_NAME} PUBLIC "/wd4267")
    target_compile_options(${PROJECT_NAME} PUBLIC "/wd4101")
endif()