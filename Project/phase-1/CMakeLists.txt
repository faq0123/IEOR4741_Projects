cmake_minimum_required(VERSION 3.10)
project(LinAlgKernels)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags: -O3 for opt, -g for debug, no -pg (causes linker errors in MinGW)
add_compile_options(-O3 -march=native -g)
# add_compile_options(-O3 -march=native -g -pg)

# Include directories
include_directories(include)

# Source files
set(SOURCES src/lin_alg.cpp)

# Test executable
add_executable(tests tests/test_basics.cpp ${SOURCES})
target_link_libraries(tests gmon)

# Benchmark executable
add_executable(benchmark benchmarks/benchmark.cpp ${SOURCES})
target_link_libraries(benchmark gmon)

# Profile-specific executable for MM
add_executable(profile_mm benchmarks/profile_mm.cpp ${SOURCES})
target_link_libraries(profile_mm gmon)