
project(hfMqttTest)

cmake_minimum_required(VERSION 2.8)

#使能c++11
set(CMAKE_CXX_STANDARD 11)
set(jsoncpp_Root ${CMAKE_SOURCE_DIR}/libs/jsoncpp)


file(GLOB jsoncpp_src_list ${jsoncpp_Root}/*.cpp ${jsoncpp_Root}/*.h )

file(GLOB src_list hfMqttManager.cpp hfHttpClient.cpp)

include_directories(${CMAKE_SOURCE_DIR}/libs/paho.mqtt.c/include)
include_directories(${CMAKE_SOURCE_DIR}/libs)
include_directories(${CMAKE_SOURCE_DIR}/libs/curl-7.55.1/include)


link_directories(${CMAKE_SOURCE_DIR}/libs/paho.mqtt.c/lib)
link_directories(${CMAKE_SOURCE_DIR}/libs/curl-7.55.1/lib)


ADD_LIBRARY(jsoncpp STATIC ${jsoncpp_src_list})

ADD_EXECUTABLE(hfMqttTest hfMqttTest.cpp)
TARGET_LINK_LIBRARIES(hfMqttTest paho-mqtt3c)



ADD_EXECUTABLE(hfMqttTest1 hfMqttTest1.cpp ${src_list})
TARGET_LINK_LIBRARIES(hfMqttTest1 paho-mqtt3c jsoncpp curl pthread)





