# CMakeLists.txt for Zorro C++ Test Scripts
# Builds test scripts as DLLs that Zorro can load

cmake_minimum_required(VERSION 3.15)

# Path to Zorro installation (override with -DZORRO_PATH=... if needed)
set(ZORRO_PATH "C:/Zorro_2.66" CACHE PATH "Path to Zorro installation")
set(ZORRO_STRATEGY_DIR "${ZORRO_PATH}/Strategy" CACHE PATH "Path to Zorro Strategy folder")

message(STATUS "Zorro path: ${ZORRO_PATH}")
message(STATUS "Zorro Strategy folder: ${ZORRO_STRATEGY_DIR}")

# Common test helpers library (static lib linked into each test DLL)
add_library(ZorroTestHelpers STATIC
    common/TestHelpers.cpp
)

target_include_directories(ZorroTestHelpers PUBLIC
    common
    ${CMAKE_SOURCE_DIR}/include  # For trading.h
)

target_compile_features(ZorroTestHelpers PUBLIC cxx_std_17)

#=============================================================================
# Helper function to create a Zorro test DLL
#=============================================================================
function(add_zorro_test TEST_NAME)
    # Create DLL from cpp file
    add_library(${TEST_NAME} SHARED
        ${TEST_NAME}.cpp
    )
    
    # Link against test helpers
    target_link_libraries(${TEST_NAME} PRIVATE
        ZorroTestHelpers
    )
    
    # Include directories
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include  # For trading.h
        common                        # For TestHelpers.h
    )
    
    # C++17 standard
    target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
    
    # Output directly to Zorro's Strategy folder
    set_target_properties(${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${ZORRO_STRATEGY_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${ZORRO_STRATEGY_DIR}"
        OUTPUT_NAME "${TEST_NAME}"
        SUFFIX ".dll"
    )
    
    # 32-bit build if Zorro is 32-bit (most common)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_compile_definitions(${TEST_NAME} PRIVATE _WIN32)
    endif()
    
    # Post-build message
    add_custom_command(TARGET ${TEST_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Test DLL: ${ZORRO_STRATEGY_DIR}/${TEST_NAME}.dll"
    )
endfunction()

#=============================================================================
# Create test DLLs
#=============================================================================

# Position tracking tests
add_zorro_test(PositionTest)

# Limit order tests
add_zorro_test(LimitOrderTest)

# Add more tests here as you create them
# add_zorro_test(MultiPositionTest)
# add_zorro_test(StopOrderTest)

message(STATUS "Configured ${CMAKE_CURRENT_SOURCE_DIR} test DLLs")
message(STATUS "Test DLLs will be built to: ${ZORRO_STRATEGY_DIR}")
