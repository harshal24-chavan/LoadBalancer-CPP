# --- 1. Basic Project Setup ---
# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Define the project name and specify it's a C++ project
project(LoadBalancer CXX)

# --- 2. C++ Standard ---
# Specify that we want to use C++17 features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 3. Find Dependencies ---
# Find the Crow library, which is required for our project to build.
# This relies on the vcpkg toolchain file to know where to look.
find_package(Crow REQUIRED)

# Find the threads library, necessary for std::mutex and Crow's multithreading
find_package(Threads REQUIRED)

# --- 4. Define the Executable ---
# Tell CMake to create an executable named "load_balancer" from main.cpp
add_executable(load_balancer main.cpp)

# --- 5. Link Dependencies ---
# Link our executable against the libraries it needs.
# Crow::Crow and Threads::Threads are "targets" provided by find_package.
# This automatically handles include paths and library files.
target_link_libraries(load_balancer PRIVATE
    Crow::Crow
    Threads::Threads
)
