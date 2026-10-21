set(USER_HOME "$ENV{HOME}")

add_library(robotState
    robotState.cc
)

target_include_directories(robotState
    PUBLIC
        "${USER_HOME}/lhh_workspace/catkin_ws/devel/include"
        "${USER_HOME}/catkin_ws/devel/include"
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${catkin_INCLUDE_DIRS}
)

target_link_libraries(robotState
    PUBLIC
        ${catkin_LIBRARIES}
)
