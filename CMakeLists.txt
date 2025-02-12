cmake_minimum_required(VERSION 3.6)
project(vorpal_engine)

file(GLOB SRC_FILES src/*.cpp src/Engine/*.cpp src/Platform/*.cpp src/external/imgui/*.cpp)

# Set the build type to Release for optimizations
# set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)

# Apply global compiler flags to all targets
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    # For GCC/Clang, set optimization flags globally
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -flto -march=native")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -flto -march=native")
elseif(MSVC)
    # For MSVC, set optimization flags globally
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /GL")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /O2 /GL")
endif()

add_executable(vorpal_engine ${SRC_FILES} external/volk/volk.c)

# libraries
add_subdirectory(external/Vulkan-Headers)
add_subdirectory(external/glfw)
add_subdirectory(external/glm)
add_subdirectory(external/assimp)

target_include_directories (vorpal_engine PUBLIC
    external/Vulkan-Headers/include
    external/glfw/include
    external/glm/
    external/tinyobjloader
    external/volk/
    external/assimp/include
    external/assimp/contrib/zip/src
)

target_link_libraries(vorpal_engine glfw glm assimp)
target_include_directories(vorpal_engine PRIVATE include)

# windows, copy dlls to target
if (WIN32)
    target_link_options(vorpal_engine PRIVATE -static)
    add_custom_command(TARGET vorpal_engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_BINARY_DIR}/external/glfw/src/glfw3.dll
            ${CMAKE_CURRENT_BINARY_DIR}/glfw3.dll)
endif()