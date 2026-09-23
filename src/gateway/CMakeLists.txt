add_library(gateway
    gateway.cc
)

target_include_directories(gateway
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)