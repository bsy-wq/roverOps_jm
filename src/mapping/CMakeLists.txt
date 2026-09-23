add_library(mapping
    maphandle.cc    
)

target_include_directories(mapping
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)