# minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# project name
project(openssl-bench)

# set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenSSL library
# Required means it will fail if not found
find_package(OpenSSL REQUIRED)

# tell compiler where to find OpenSSL header files
include_directories(${PROJECT_SOURCE_DIR}/include)

# create executable named "bench" from source files
add_executable(bench
    src/bench.cpp
    src/crypto_utils.cpp
)

# link OpenSSL crypto library to bench executable
target_link_libraries(bench OpenSSL::Crypto)
