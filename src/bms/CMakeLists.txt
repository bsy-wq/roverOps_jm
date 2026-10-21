set(USER_HOME "$ENV{HOME}")

add_library(bms
    BmsForwarder.cc
    BatteryProvider.cc
)

find_path(A10_BMS_INCLUDE_DIR bms_ros_bridge/BmsStatus.h
    HINTS ${catkin_INCLUDE_DIRS}
    PATH_SUFFIXES include)
if(A10_BMS_INCLUDE_DIR)
    target_include_directories(bms PRIVATE ${A10_BMS_INCLUDE_DIR})
    target_compile_definitions(bms PRIVATE HAVE_A10_BMS=1)
else()
    message(STATUS "A10 BMS disabled: source bms_ros_bridge workspace and reconfigure to enable")
endif()

target_include_directories(bms
    PUBLIC
        "${USER_HOME}/lhh_workspace/catkin_ws/devel/include"
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bms
    PUBLIC
        ${catkin_LIBRARIES}
        Threads::Threads
)