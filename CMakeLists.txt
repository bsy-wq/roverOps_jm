cmake_minimum_required(VERSION 3.16)

project(robot_dog)

set(CMAKE_CXX_STANDARD 17)

set(USER_HOME "$ENV{HOME}")

find_package(catkin REQUIRED COMPONENTS
    roscpp
    geometry_msgs
    nav_msgs
    rosgraph_msgs
    sensor_msgs
    std_msgs
    tf
)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Iconv REQUIRED)
find_package(CURL REQUIRED)
find_package(tinyxml2 REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(LibXml2 REQUIRED)

include_directories(
    ${catkin_INCLUDE_DIRS}
    ${YAML_CPP_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(EXISTS "${USER_HOME}/catkin_ws/devel/include/robot_gateway")
    include_directories("${USER_HOME}/catkin_ws/devel/include")
endif()

# 模块
add_subdirectory(src/bms)
add_subdirectory(src/gateway)
add_subdirectory(src/mapping)
add_subdirectory(src/hikvision)
add_subdirectory(src/robotState)
add_subdirectory(src/modifyValue)
add_subdirectory(src/motionControl)
add_subdirectory(src/broadcast)
add_subdirectory(src/follow)

# Run against an isolated ROS master; this target never runs during a normal build.
add_executable(battery_provider_integration_test EXCLUDE_FROM_ALL
    tests/battery_integration_test.cc)
target_link_libraries(battery_provider_integration_test
    bms robotState ${catkin_LIBRARIES} Threads::Threads OpenSSL::SSL OpenSSL::Crypto)
if(A10_BMS_INCLUDE_DIR)
    target_include_directories(battery_provider_integration_test PRIVATE ${A10_BMS_INCLUDE_DIR})
    target_compile_definitions(battery_provider_integration_test PRIVATE HAVE_A10_BMS=1)
endif()

# Behavior priority matrix; no ROS or hardware required.
add_executable(robot_fsm_priority_test EXCLUDE_FROM_ALL
    tests/robot_fsm_priority_test.cc)
target_link_libraries(robot_fsm_priority_test Threads::Threads)

add_executable(${PROJECT_NAME}
    src/main.cc
    src/CommandExecutor.cc
    src/handlers/NavigationLifecycleHandler.cc
    src/handlers/MapWorkflowHandler.cc
    src/handlers/DeviceHandler.cc
    src/localWebsocket/LocalWebsocketServer.cc
    src/aiRecognition/AiRecognitionAckHandler.cc
    # src/BmsForwarder.cc
)

target_link_libraries(${PROJECT_NAME}
    ${catkin_LIBRARIES}
    ${YAML_CPP_LIBRARIES}
    Threads::Threads
    uuid
    ssh2
    OpenSSL::Crypto
    Iconv::Iconv
    CURL::libcurl
    tinyxml2::tinyxml2
    LibXml2::LibXml2

    bms
    gateway
    mapping
    hikvision
    robotState
    modifyValue
    motionControl
    broadcast
    follow
)
