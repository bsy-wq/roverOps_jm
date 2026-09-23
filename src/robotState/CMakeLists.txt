add_library(robotState
    robotState.cc
)

target_include_directories(robotState
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${catkin_INCLUDE_DIRS}
        /home/jianmi/catkin_ws/devel/include
)

target_link_libraries(robotState
    PUBLIC
        ${catkin_LIBRARIES}
)
