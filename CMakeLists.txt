cmake_minimum_required(VERSION 3.6)
project(vorpal_engine)

# static windows linking
if (WIN32)
    set(CMAKE_CXX_FLAGS, -static -g3 -gdwarf-4 )
endif()

# bullet2 demos break on windows builds, not needed for project
set(BUILD_BULLET2_DEMOS, OFF)
set(BUILD_UNIT_TESTS, OFF)
set(BUILD_OPENGL3_DEMOS, OFF)
set(BUILD_CPU_DEMOS, OFF)
set(ASSIMP_BUILD_TESTS, OFF)
set(ASSIMP_BUILD_DLL_EXPORT, ON)

file(GLOB SRC_FILES src/*.cpp src/Engine/*.cpp src/Engine/*.c src/Platform/*.cpp src/external/imgui/*.cpp)

# Track every file under assets/ so any change triggers a zip rebuild
file(GLOB_RECURSE ASSET_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")

# At build time: purge stale copy, recopy from source, repack zip
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/assets.zip"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_CURRENT_BINARY_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${CMAKE_CURRENT_BINARY_DIR}/assets"
    COMMAND zip -0 -r "${CMAKE_CURRENT_BINARY_DIR}/assets.zip" assets
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS ${ASSET_SOURCE_FILES}
    COMMENT "Repacking assets.zip"
    VERBATIM
)
add_custom_target(assets_zip ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/assets.zip")

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

add_executable(vorpal_engine
    ${SRC_FILES}
    # volk
    external/volk/volk.c
    
    # imgui
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_vulkan.cpp
)

# libraries
add_subdirectory(external/Vulkan-Headers)
add_subdirectory(external/glfw)
add_subdirectory(external/glm)
add_subdirectory(external/bullet3)
add_subdirectory(external/tinygltf)
add_subdirectory(external/miniz)
add_subdirectory(external/lua)

set(PA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(PA_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
add_subdirectory(external/portaudio)

target_include_directories (vorpal_engine PUBLIC
    external/Vulkan-Headers/include
    external/glfw/include
    external/glm/
    external/volk/
    external/imgui
    external/bullet3/src/
    external/tinygltf/
    external/miniz
    external/lua
    external/sol2/include
    external/portaudio/include
    external/
    ${CMAKE_CURRENT_BINARY_DIR}/external/miniz
)

#target_compile_definitions(vorpal_engine PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK=1 VK_USE_PLATFORM_WAYLAND_KHR=1)
target_compile_definitions(vorpal_engine PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK=1)

set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)

#find_package(Bullet REQUIRED)
#target_link_libraries(vorpal_engine glfw glm assimp ${BULLET_LIBRARIES} wayland-client xkbcommon xdg-shell)
target_link_libraries(vorpal_engine glm Bullet3Common Bullet3Collision BulletDynamics Bullet3Dynamics BulletCollision Bullet3Geometry LinearMath glfw miniz tinygltf lua portaudio)
target_include_directories(vorpal_engine PRIVATE include)
target_compile_options(vorpal_engine PRIVATE -ffat-lto-objects -flto -O3 -ffast-math)

# windows, copy dlls to target
if (WIN32)
    target_link_options(vorpal_engine PRIVATE -static -g3 -gdwarf-4 )
endif()

target_compile_features(vorpal_engine PRIVATE cxx_std_17)

add_dependencies(vorpal_engine assets_zip)