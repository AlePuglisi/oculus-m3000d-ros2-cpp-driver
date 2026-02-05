cmake_minimum_required(VERSION 3.5)

project(oculus_app)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -ggdb3)
endif()



include_directories( oculus_app
        include
        /usr/local/include) 

find_package( OpenCV REQUIRED )
find_package(oculus_driver REQUIRED)
find_package(Eigen3 REQUIRED)

add_executable(oculus_app main.cpp src/OculusApp.cpp src/Logger.cpp)

target_link_libraries(oculus_app PRIVATE
    oculus_driver
    ${OpenCV_LIBS} 
)

target_include_directories(oculus_app PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${OpenCV_INCLUDE_DIRS}>
    $<INSTALL_INTERFACE:include>
)