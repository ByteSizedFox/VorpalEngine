cmake_minimum_required(VERSION 3.6)
project(vorpal_engine)

file(GLOB SRC_FILES src/*.cpp src/Engine/*.cpp src/Platform/*.cpp src/external/imgui/*.cpp)

add_executable(vorpal_engine ${SRC_FILES} external/volk/volk.c)

# libraries
add_subdirectory(external/Vulkan-Headers)
add_subdirectory(external/glfw)
add_subdirectory(external/glm)
add_subdirectory(external/volk)

target_include_directories (vorpal_engine PUBLIC
    external/Vulkan-Headers/include
    external/glfw/include
    external/glm/
    external/tinyobjloader
    external/volk/
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