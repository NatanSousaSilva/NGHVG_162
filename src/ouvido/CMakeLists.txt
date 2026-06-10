cmake_minimum_required(VERSION 3.8)
project(ouvido)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(CURL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

add_executable(ouvido src/ouvido_node.cpp)

target_include_directories(ouvido PRIVATE ${PORTAUDIO_INCLUDE_DIRS})

ament_target_dependencies(ouvido
  rclcpp
  std_msgs
)

target_link_libraries(ouvido
  CURL::libcurl
  ${PORTAUDIO_LIBRARIES}
)

install(TARGETS
  ouvido
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
