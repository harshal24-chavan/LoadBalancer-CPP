# 1. ALWAYS put cmake_minimum_required at the very top!
cmake_minimum_required(VERSION 3.15)

# Define the project name
project(LoadBalancer CXX)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include the FetchContent module
include(FetchContent)

# ------------------------------------------------------------------
# DEPENDENCIES (FetchContent)
# ------------------------------------------------------------------

# 1. tomlplusplus
message(STATUS "Fetching tomlplusplus...")
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

# 2. efsw (File Watcher for Hot-Reload)
message(STATUS "Fetching efsw...")
FetchContent_Declare(
    efsw
    GIT_REPOSITORY https://github.com/SpartanJ/efsw.git
    GIT_TAG        master 
)
FetchContent_MakeAvailable(efsw)

# 3. spdlog (High-performance logging)
message(STATUS "Fetching spdlog...")
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.12.0 
)
FetchContent_MakeAvailable(spdlog)

# ------------------------------------------------------------------
# CORE PROJECT BUILD
# ------------------------------------------------------------------

add_executable(
    load_balancer
    src/main.cpp
    src/loadbalancer.cpp
    src/RouteStrategy.cpp
    src/Server.cpp
    src/tomlParser.cpp
    #src/healthChecker.cpp
    src/HotReloader.cpp
    src/MetricsCalc.cpp
    #src/ResponseDecorator.cpp
    src/IoUringEngine.cpp
)

# Set include directories
target_include_directories(load_balancer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Find standard threads library (Required by efsw, spdlog, and your health checker)
find_package(Threads REQUIRED)

# Link the executable against the required libraries
target_link_libraries(load_balancer PRIVATE
    tomlplusplus::tomlplusplus
    spdlog::spdlog
    efsw
    Threads::Threads
    uring 
)
