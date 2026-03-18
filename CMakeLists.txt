cmake_minimum_required(VERSION 3.16)
project(TFV VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect sources
set(SOURCES
    src/main.cpp
    src/app/App.cpp
    src/ui/Renderer.cpp
    src/ui/CommandLine.cpp
    src/ui/LiveSearch.cpp
    src/fs/FileSystem.cpp
    src/config/Config.cpp
    src/platform/ConsoleWindows.cpp
)

add_executable(tfv ${SOURCES})

target_include_directories(tfv PRIVATE src)

# Windows-specific libraries
if(WIN32)
    target_compile_definitions(tfv PRIVATE PLATFORM_WINDOWS)
    target_link_libraries(tfv PRIVATE shlwapi)
endif()

# Output binary to build/
set_target_properties(tfv PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
