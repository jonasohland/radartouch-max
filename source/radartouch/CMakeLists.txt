cmake_minimum_required(VERSION 3.0)

find_package(Boost)

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/min-api)

include(${C74_MIN_API_DIR}/script/min-pretarget.cmake)

add_library(
    ${PROJECT_NAME}
    MODULE
    ${PROJECT_NAME}.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/oscpp/include)

target_include_directories(${PROJECT_NAME} PUBLIC ${C74_INCLUDES})

target_include_directories(${PROJECT_NAME} PUBLIC ${Boost_INCLUDE_DIRS})

include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)
