add_library(follow
    followHandle.cc
)

target_link_libraries(follow
    PUBLIC 
        CURL::libcurl
)
