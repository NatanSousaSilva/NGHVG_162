cmake_minimum_required(VERSION 3.8)
project(boca)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

find_package(CURL REQUIRED)

add_executable(boca src/boca_node.cpp)

ament_target_dependencies(boca
  rclcpp
  std_msgs
)

target_link_libraries(boca
  CURL::libcurl
)

install(TARGETS
  boca
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
