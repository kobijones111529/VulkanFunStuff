cmake_minimum_required(VERSION 3.0.0)
project(VulkanFunStuff VERSION 0.1.0)

include(CTest)
enable_testing()

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SOURCES
    src/main.cpp
    src/Utils.cpp
    src/MainWindow.cpp
    src/Application.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${GLFW_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE include)

target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARY})

add_custom_command(TARGET ${PROJECT_NAME} COMMAND ${CMAKE_SOURCE_DIR}/compile_shaders.sh)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
