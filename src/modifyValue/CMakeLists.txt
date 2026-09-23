add_library(modifyValue
    modifyHandle.cc
)

target_include_directories(modifyValue
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)