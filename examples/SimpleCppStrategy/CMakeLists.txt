# CMakeLists.txt - Example C++ Zorro Strategy
# Demonstrates proper C++ strategy development workflow

cmake_minimum_required(VERSION 3.15)
project(SimpleCppStrategy VERSION 1.0.0 LANGUAGES CXX)

# Configuration
set(ZORRO_PATH "C:/Zorro_2.66" CACHE PATH "Path to Zorro installation")
set(ZORRO_STRATEGY_DIR "${ZORRO_PATH}/Strategy" CACHE PATH "Zorro Strategy output folder")
set(ZORRO_INCLUDE_DIR "${ZORRO_PATH}/include" CACHE PATH "Zorro include directory")
set(ZORRO_LIB_DIR "${ZORRO_PATH}/lib" CACHE PATH "Zorro library directory")

message(STATUS "Zorro Path: ${ZORRO_PATH}")
message(STATUS "Output Directory: ${ZORRO_STRATEGY_DIR}")

# CRITICAL: Must build 32-bit for Zorro (unless using 64-bit Zorro)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(WARNING "Building 64-bit, but most Zorro installations are 32-bit!")
    message(WARNING "Use: cmake -A Win32 .. if Zorro is 32-bit")
    set(ZORRO_LIB "${ZORRO_LIB_DIR}/zorro_x64.lib")
else()
    set(ZORRO_LIB "${ZORRO_LIB_DIR}/zorro.lib")
endif()

# Check if zorro.lib exists
if(NOT EXISTS "${ZORRO_LIB}")
    message(WARNING "zorro.lib not found at ${ZORRO_LIB}")
    message(WARNING "You may need to extract it from Zorro or use szabonorbert's template")
endif()

# Create strategy DLL
add_library(SimpleCppStrategy SHARED
    SimpleCppStrategy.cpp
)

# Link against Zorro API library
target_link_libraries(SimpleCppStrategy 
    PRIVATE "${ZORRO_LIB}"
)

# Include Zorro headers
target_include_directories(SimpleCppStrategy 
    PRIVATE 
        "${ZORRO_INCLUDE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

# C++17 standard
target_compile_features(SimpleCppStrategy PRIVATE cxx_std_17)

# DLL configuration
set_target_properties(SimpleCppStrategy PROPERTIES
    PREFIX ""                                   # No "lib" prefix
    SUFFIX ".dll"                              # .dll extension
    RUNTIME_OUTPUT_DIRECTORY "${ZORRO_STRATEGY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${ZORRO_STRATEGY_DIR}"
)

# Post-build message
add_custom_command(TARGET SimpleCppStrategy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Strategy DLL: ${ZORRO_STRATEGY_DIR}/SimpleCppStrategy.dll"
    COMMAND ${CMAKE_COMMAND} -E echo "Load 'SimpleCppStrategy.dll' in Zorro to run"
)

# Optional: Copy to Zorro on build
# Uncomment if you want automatic deployment
# add_custom_command(TARGET SimpleCppStrategy POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy 
#         "$<TARGET_FILE:SimpleCppStrategy>" 
#         "${ZORRO_STRATEGY_DIR}/SimpleCppStrategy.dll"
# )
