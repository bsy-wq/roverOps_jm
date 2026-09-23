add_library(hikvision
    hikvihandle.cc
)

target_include_directories(hikvision
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${LIBXML2_INCLUDE_DIRS}
        ${LIBXML2_INCLUDE_DIR}
)

target_link_libraries(hikvision
    PUBLIC
        CURL::libcurl
        ${LIBXML2_LIBRARIES}
        ${LIBXML2_LIBRARY}
        Threads::Threads
)
