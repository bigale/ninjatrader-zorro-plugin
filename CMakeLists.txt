# CMakeLists.txt - NinjaTrader 8 Zorro Plugin Build Configuration
# 
# Build Instructions:
#   mkdir build && cd build
#   cmake -G "Visual Studio 17 2022" -A Win32 ..
#   cmake --build . --config Release
#
# Or open in Visual Studio directly

cmake_minimum_required(VERSION 3.15)
project(NT8Plugin VERSION 1.0.0 LANGUAGES CXX)

# Must be 32-bit for Zorro compatibility
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(WARNING "Building 64-bit, but Zorro requires 32-bit DLL!")
    message(WARNING "Use: cmake -A Win32 ..")
endif()

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7+
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Source files
set(SOURCES
    src/NT8Plugin.cpp
    src/TcpBridge.cpp
)

# Header files
set(HEADERS
    include/NT8Plugin.h
    include/TcpBridge.h
    include/trading.h
)

# Create DLL
add_library(NT8 SHARED ${SOURCES} ${HEADERS} src/NT8Plugin.def)

# Include directories
target_include_directories(NT8 PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Output name without lib prefix
set_target_properties(NT8 PROPERTIES
    PREFIX ""
    OUTPUT_NAME "NT8"
)

# Post-build: copy to Zorro Plugin folder (uncomment and adjust path)
# add_custom_command(TARGET NT8 POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:NT8> "C:/Zorro/Plugin/NT8.dll"
#     COMMENT "Copying NT8.dll to Zorro Plugin folder"
# )

# Install target
install(TARGETS NT8
    RUNTIME DESTINATION Plugin
)

# Option to build Zorro C++ test scripts (DISABLED - has compilation errors)
option(BUILD_ZORRO_TESTS "Build Zorro C++ test DLLs" OFF)

if(BUILD_ZORRO_TESTS)
    message(STATUS "Building Zorro C++ test scripts")
    add_subdirectory(zorro-tests)
endif()

# Option to build C++ strategy examples
option(BUILD_EXAMPLES "Build example C++ strategies" OFF)

if(BUILD_EXAMPLES)
    message(STATUS "Building C++ strategy examples")
    add_subdirectory(examples/SimpleCppStrategy)
endif()
