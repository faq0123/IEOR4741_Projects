cmake_minimum_required(VERSION 3.10)
project(HFT_Project VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags for optimization
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/MarketData.cpp
    src/OrderBook.cpp
    src/MatchingEngine.cpp
    src/OrderManager.cpp
    src/TradeLogger.cpp
)

# Main executable
add_executable(hft_app
    src/main.cpp
    ${SOURCES}
)

# Latency test executable
add_executable(test_latency
    test/test_latency.cpp
    ${SOURCES}
)

# Set output directories
set_target_properties(hft_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)

set_target_properties(test_latency PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
)

# Platform-specific settings
if(APPLE)
    # macOS specific settings
    target_compile_definitions(hft_app PRIVATE MACOS_BUILD)
    target_compile_definitions(test_latency PRIVATE MACOS_BUILD)
elseif(UNIX)
    # Linux specific settings
    find_package(Threads REQUIRED)
    target_link_libraries(hft_app Threads::Threads)
    target_link_libraries(test_latency Threads::Threads)
elseif(WIN32)
    # Windows specific settings
    target_compile_definitions(hft_app PRIVATE WINDOWS_BUILD)
    target_compile_definitions(test_latency PRIVATE WINDOWS_BUILD)
endif()

# Enable testing
enable_testing()
add_test(NAME LatencyBenchmark COMMAND test_latency)

# Print build configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler flags: ${CMAKE_CXX_FLAGS}")
if(CMAKE_BUILD_TYPE MATCHES Release)
    message(STATUS "Release flags: ${CMAKE_CXX_FLAGS_RELEASE}")
elseif(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Debug flags: ${CMAKE_CXX_FLAGS_DEBUG}")
endif()
