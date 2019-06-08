generator_is_ide(IS_IDE)

add_library(
    radartouch_shared_headers
    INTERFACE
)

target_include_directories(radartouch_shared_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(radartouch_shared_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/oscpp/include)


if(${IS_IDE})

    add_library(
        radartouch_shared
        STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/radartouch_receiver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/radartouch_message.h
    )

    set_target_properties(radartouch_shared PROPERTIES LINKER_LANGUAGE CXX)

endif()