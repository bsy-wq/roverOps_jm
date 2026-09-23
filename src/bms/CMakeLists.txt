add_library(bms
    BmsForwarder.cc
)

target_include_directories(bms
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bms
    PUBLIC
        ${catkin_LIBRARIES}
        Threads::Threads
)
