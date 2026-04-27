cmake_minimum_required(VERSION 3.8)
project(localizacao)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(localizacao_node src/localizacao_node.cpp)

ament_target_dependencies(localizacao_node
  rclcpp
  std_msgs
)

install(TARGETS
  localizacao_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
