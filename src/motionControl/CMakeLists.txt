add_library(motionControl
    motionHandle.cc
)

target_include_directories(motionControl
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)
