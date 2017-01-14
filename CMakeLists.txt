cmake_minimum_required(VERSION 3.6)
project(vulkan_engine)

set(CMAKE_CXX_STANDARD 11)

find_package(PkgConfig REQUIRED)
find_package(Vulkan REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

set(SOURCE_FILES main.cpp engine/Vulkan/VDeleter.h engine/Vulkan/Vulkan.cpp engine/Vulkan/Vulkan.h engine/Vulkan/Window.h engine/util.h)
add_executable(vulkan_engine ${SOURCE_FILES})

target_include_directories(vulkan_engine PUBLIC
        ${VULKAN_INCLUDE_DIRS}
        )

target_link_libraries(vulkan_engine PUBLIC
        ${Vulkan_LIBRARIES}
        ${GLFW_LIBRARIES}
        )
