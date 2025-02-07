cmake_minimum_required(VERSION 3.13)
project(vorpal_engine)

file(GLOB SRC_FILES src/*.cpp src/Engine/*.cpp src/Platform/*.cpp)

add_executable(vorpal_engine ${SRC_FILES} src/external/volk/volk.c)

# libraries
add_subdirectory(src/external/Vulkan-Headers)
add_subdirectory(src/external/glfw)
add_subdirectory(src/external/glm)

target_include_directories (vorpal_engine PUBLIC
    src/external/Vulkan-Headers/include
    src/external/glfw/include
    src/external/volk/
    src/external/glm/
)

target_link_libraries(vorpal_engine glfw glm)
target_include_directories(vorpal_engine PRIVATE include)

# windows, copy dlls to target
if (WIN32)
    target_link_options(vorpal_engine PRIVATE -static)
    add_custom_command(TARGET vorpal_engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_BINARY_DIR}/src/external/glfw/src/glfw3.dll
            ${CMAKE_CURRENT_BINARY_DIR}/glfw3.dll)
endif()