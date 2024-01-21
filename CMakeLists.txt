cmake_minimum_required(VERSION 3.0...3.25)
project(
    Nostalgia
    VERSION 0.0.1
    LANGUAGES CXX C
)

option(DEV_MODE "Set up development helper settings" ON)

add_subdirectory(external/glfw)
add_subdirectory(external/dawn)
add_subdirectory(external/glfw3webgpu)
add_subdirectory(external/glm)
include_directories(external/json/include)
include_directories(external/stb)

add_executable(nostalgia
    src/engine/engine.h
    src/engine/engine.cpp
    src/files/shader_loader.h
    src/files/shader_loader.cpp
    src/files/texture_loader.h
    src/files/texture_loader.cpp
    src/files/tilemap_loader.h
    src/files/tilemap_loader.cpp
    src/files/geometry_loader.h
    src/files/geometry_loader.cpp
    src/implementations.cpp
    src/nostalgia.cpp
)

target_link_libraries(nostalgia PRIVATE glfw webgpu glfw3webgpu glm)

set_target_properties(nostalgia PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR ON
)

if(DEV_MODE)
	target_compile_definitions(nostalgia PRIVATE
		RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
	)

else()
	target_compile_definitions(nostalgia PRIVATE
		RESOURCE_DIR="./resources"
	)
endif()

if (MSVC)
    target_compile_options(nostalgia PRIVATE /W4)
else()
    target_compile_options(nostalgia PRIVATE -Wall -Wextra -pedantic)
endif()

if(XCODE)
    set_target_properties(nostalgia PROPERTIES
        XCODE_GENERATE_SCHEME ON
        XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal")
endif()