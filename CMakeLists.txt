cmake_minimum_required(VERSION 3.20)

project(robot_dog)

set(CMAKE_CXX_STANDARD 17)

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

if(EXISTS "/home/jianmi/catkin_ws/devel/include/robot_gateway")
    include_directories(/home/jianmi/catkin_ws/devel/include)
endif()

# 模块
add_subdirectory(src/bms)
add_subdirectory(src/gateway)
add_subdirectory(src/mapping)
add_subdirectory(src/navigation)
add_subdirectory(src/hikvision)
add_subdirectory(src/robotState)
add_subdirectory(src/modifyValue)
add_subdirectory(src/motionControl)

add_executable(${PROJECT_NAME}
    src/main.cc
    src/CommandExecutor.cc
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
    navigation
    hikvision
    robotState
    modifyValue
    motionControl
)
