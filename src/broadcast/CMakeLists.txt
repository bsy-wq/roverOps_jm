add_library(broadcast
    broadcastHandle.cc
)

target_link_directories(broadcast
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)