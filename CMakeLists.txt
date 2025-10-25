# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Define the project name
project(LoadBalancer)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the executable and list all your source (.cpp) files
add_executable(
    load_balancer
    src/main.cpp
    src/loadbalancer.cpp
    src/RouteStrategy.cpp
    src/Server.cpp
    src/tomlParser.cpp
    src/healthChecker.cpp
    src/HotReloader.cpp
    src/MetricsCalc.cpp
    src/ResponseDecorator.cpp
    src/RequestHandler.cpp
)



target_include_directories(load_balancer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)


# Find the Crow library dependency (via vcpkg)
find_package(Crow REQUIRED)

# Find the standard threads library
find_package(Threads REQUIRED)

# Find the toml++ library
find_package(tomlplusplus REQUIRED)

# Find the cpr library
find_package(cpr REQUIRED)
# find the efsw library for file watching
find_package(efsw REQUIRED)

find_package(spdlog REQUIRED)



# Link the executable against the required libraries
target_link_libraries(load_balancer PRIVATE
    Crow::Crow
    tomlplusplus::tomlplusplus
    Threads::Threads
    cpr::cpr
    efsw::efsw
    spdlog::spdlog
)
