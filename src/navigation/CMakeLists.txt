add_library(navigation
    navhandle.cc    
)

target_include_directories(navigation
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)