# C++ Application Layer (L7) Load Balancer

![Load Balancer](https://external-content.duckduckgo.com/iu/?u=https%3A%2F%2Ftse2.mm.bing.net%2Fth%2Fid%2FOIP.zMTcLwRh-eSQ_i8nrKFv6QHaFW%3Fpid%3DApi&f=1&ipt=72b7977ce53c68c6169ebd581512284fa0ed7546fc09c6e36e2a08e6b4fde5f7&ipo=images)

#### A high-performance, feature-rich HTTP/S reverse proxy and load balancer written in modern C++. This project is built using Low-Level Design (LLD) principles to be scalable, maintainable, and robust.

### 📖 Description

This project is a Layer 7 (Application Layer) load balancer that functions as a reverse proxy for HTTP/S traffic. It distributes incoming requests across a pool of backend servers using various balancing algorithms.

It is built from the ground up with a focus on clean architecture and modern C++ practices. It leverages powerful libraries like Crow (C++ Micro Web Framework) for request handling and CPR (C++ Requests) for proxying. The design heavily features common software design patterns for scalability and non-intrusive feature addition.

### ✨ Features

**HTTP/S Reverse Proxy**: Manages and forwards incoming HTTP/S requests to configured backend servers.

**LLD-Driven Architecture**:
- Singleton Pattern: Ensures a single, globally accessible instance for core services like the LoadBalancer and Metrics manager.
- Decorator Pattern: Enables non-intrusive, scalable features like request logging (ResponseDecorator) and metrics collection (MetricsCalc).

**Zero-Downtime Configuration**:
- **Fully Config-Driven**: All settings, including server pools and algorithms, are managed via a TOML file (tomlParser.cpp).
- **Hot-Reloading**: Uses an EFSW file watcher (HotReloader.cpp) to automatically detect and apply configuration changes (e.g., adding/removing servers) without a restart.

**Health Checks**: Employs multi-threaded, asynchronous health checks (healthChecker.cpp) to continuously monitor backend servers. Automatically removes unhealthy servers from the rotation and re-adds them upon recovery.

**Multiple Balancing Algorithms**:
- Round Robin
- Least Connections
(Implemented via RouteStrategy.cpp)

**Modern C++ Stack**: Built using crow, cpr-c++, tomlplusplus and efsw.

### 🛠️ Prerequisites

Before you begin, ensure you have the following installed:

- A C++ compiler (e.g., g++ or clang++) supporting C++20 or newer.
- CMake (version 3.10 or newer)
- Crow library (libcrow-dev)
- CPR library (libcpr-dev)
- TOML++ library (libtomlplusplus-dev)
- EFSW library (libefsw-dev)
- OpenSSL (libssl-dev)

### Example for Ubuntu/Debian
#### Note: You may need to build some of these from source or use a package manager like vcpkg
```bash
sudo apt-get update
sudo apt-get install build-essential g++ cmake libssl-dev

```
### Install other dependencies as required (e.g., vcpkg install crow cpr tomlplusplus efsw)


🚀 Building the Project

This project uses CMake for building.

##### Clone the repository:

```bash
git clone https://your-repo-url/cpp-load-balancer.git
cd cpp-load-balancer

```

#### Configure and build with CMake:

##### Create a build directory
```bash
cmake -B build
```
##### Compile the project
```bash
cmake --build build
```

##### This will create the load_balancer executable inside the build/ directory.

### 🏃 Usage

The load balancer is fully driven by its configuration file : **config.toml**

Run the application:
```bash
./build/load_balancer
```

The application will start, load the config.toml, and begin monitoring it for changes.

### Example config.toml
``` toml
[load_balancer]
# Port for the load balancer to listen on
listen_port = 8080
# Balancing algorithm: "RoundRobin" | "LeastConnections"
algorithm = "RoundRobin"

# Health check configuration
[health_check]
# URI to check on backend servers
path = "/health"
# Interval between checks in seconds
interval = 10

# Define your backend servers
[[servers]]
name = "Server 1"
url = "[http://127.0.0.1:3000](http://127.0.0.1:3000)"

[[servers]]
name = "Server 2"
url = "[http://127.0.0.1:3001](http://127.0.0.1:3001)"

[[servers]]
name = "Server 3"
url = "[http://127.0.0.1:3002](http://127.0.0.1:3002)"
```

### 📂 Project Structure
```
.
├── src/
│   ├── main.cpp                # Main entry point
│   ├── loadbalancer.cpp        # Core singleton, handles forwarding
│   ├── RequestHandler.cpp      # Base request handler implementation
│   ├── RouteStrategy.cpp       # Interface/Implementation for balancing algorithms
│   ├── Server.cpp              # Represents a backend server
│   ├── tomlParser.cpp          # Handles loading/reloading config.toml
│   ├── healthChecker.cpp       # Manages async health checks
│   ├── HotReloader.cpp         # File watcher for hot-reloading config
│   ├── MetricsCalc.cpp         # Decorator/class for collecting metrics
│   └── ResponseDecorator.cpp   # Decorator for request logging
├── include/
│   └── (All .h or .hpp files)
├── CMakeLists.txt              # Build script
├── config.toml                 # Example configuration file
└── README.md                   # This file
```
