cmake_minimum_required(VERSION 3.16)
project(hft_crtp_assignment LANGUAGES CXX)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to Release build if not set
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler optimizations (for GCC/Clang)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-O3 -DNDEBUG -march=native -fno-exceptions -fno-rtti)
endif()

# Define executable target
add_executable(hft
  src/main.cpp
)

# Add include directory
target_include_directories(hft PRIVATE include)
