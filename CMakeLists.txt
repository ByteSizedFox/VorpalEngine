cmake_minimum_required(VERSION 3.6)
project(vorpal_engine)

# static windows linking
if (WIN32)
set(CMAKE_CXX_FLAGS, -ffat-lto-objects -flto -static)
endif()

file(GLOB SRC_FILES src/*.cpp src/Engine/*.cpp src/Platform/*.cpp src/external/imgui/*.cpp)

# First, gather all files while preserving relative paths from the assets directory
file(GLOB_RECURSE ASSET_FILES 
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/*"
)

# Create a clean directory for staging
file(REMOVE_RECURSE "${CMAKE_CURRENT_BINARY_DIR}/assets")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/assets")

# Copy files to staging with clean paths
foreach(ASSET ${ASSET_FILES})
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/${ASSET}"
        "${CMAKE_CURRENT_BINARY_DIR}/assets/${ASSET}"
        COPYONLY
    )
endforeach()

# Create the zip archive from the staged directory with minimal compression
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/assets.zip"
    COMMAND zip -0 -r "${CMAKE_CURRENT_BINARY_DIR}/assets.zip" assets
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    COMMENT "Creating assets.zip with minimal compression for faster decompression"
    VERBATIM
)

# Copy to a simple name in the binary dir
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/a.bin"
    COMMAND ${CMAKE_COMMAND} -E copy 
        "${CMAKE_CURRENT_BINARY_DIR}/assets.zip"
        "${CMAKE_CURRENT_BINARY_DIR}/a.bin"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/assets.zip"
    COMMENT "Copying to simple filename"
    VERBATIM
)

# Use llvm-objcopy to convert the zip to an object file
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/assets.o"
    COMMAND cd "${CMAKE_CURRENT_BINARY_DIR}" && ${CMAKE_OBJCOPY} 
        -I binary
        -O elf64-x86-64
        -B i386:x86-64
        a.bin 
        assets.o
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/a.bin"
    COMMENT "Converting assets.zip to object file"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
)

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
    ${CMAKE_CURRENT_BINARY_DIR}/assets.o
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
add_subdirectory(external/assimp)
#add_subdirectory(external/bullet3)

target_include_directories (vorpal_engine PUBLIC
    external/Vulkan-Headers/include
    external/glfw/include
    external/glm/
    external/volk/
    external/assimp/include
    external/assimp/contrib/zip/src
    external/imgui
    external/bullet3/src/
)
target_compile_definitions(vorpal_engine PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK=1)

find_package(Bullet REQUIRED)
target_link_libraries(vorpal_engine glfw glm assimp ${BULLET_LIBRARIES})
target_include_directories(vorpal_engine PRIVATE include)

# windows, copy dlls to target
if (WIN32)
    target_link_options(vorpal_engine PRIVATE -static)
endif()
