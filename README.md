![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)

# Zero-Copy C++ L7 Load Balancer (Powered by io_uring)

![Load Balancer](https://external-content.duckduckgo.com/iu/?u=https%3A%2F%2Ftse2.mm.bing.net%2Fth%2Fid%2FOIP.zMTcLwRh-eSQ_i8nrKFv6QHaFW%3Fpid%3DApi&f=1&ipt=72b7977ce53c68c6169ebd581512284fa0ed7546fc09c6e36e2a08e6b4fde5f7&ipo=images)

#### A high-performance HTTP reverse proxy and load balancer written in modern C++. Architected around Linux io_uring to achieve a zero-copy, zero-allocation data plane.

### 📖 Description

This project is a Layer 7 load balancer designed to minimize userspace overhead by interfacing directly with the Linux Kernel API. 

By leveraging `io_uring` features (multishot accept, fixed file descriptors), pre-allocated kernel pipe pools, and `splice()`-based data forwarding, this proxy routes HTTP traffic without copying payload data into userspace memory or performing dynamic allocations during active requests.
### 🚀 Performance Characteristics
This proxy is architected to completely eliminate kernel/userspace bottlenecks. The core loop operates with strictly Zero-Copy data routing and Zero-Allocation memory management.
#### Benchmark (wrk)
Tested with 50 concurrent connections against local Go backends:
```text
Requests/sec:    34,700.00+
Latency (Avg):   ~2.04ms
CPU Utilization: 32%
Socket errors:   connect 0, read 0, write 0, timeout 0
```

### Kernel CPU Profiling (perf FlameGraph)
A live perf record during maximum load proves the zero-copy and zero-allocation claims. The CPU spends the vast majority of its time executing core io_uring system calls and processing the Linux TCP/IP stack, with completely negligible userspace overhead.
![FlameGraph](https://raw.githubusercontent.com/harshal24-chavan/LoadBalancer-CPP/refs/heads/main/proxy-flamegraph.svg)
#### Key Optimizations Verified by Profiling:
- **No sys_pipe2 or sys_close:** A custom PipePool entirely eliminates dynamic file descriptor creation under load.
- **No Userspace memcpy:** Splicing between fixed backend sockets and fixed kernel pipes keeps all payload data strictly inside the kernel.
- **No Userspace malloc:** Strict std::unique_ptr reuse and pre-allocated arrays ensure the C++ heap remains completely untouched during request processing.

### ✨ Core Features

**Kernel-Level Traffic Routing (io_uring):**
- Completely bypasses standard epoll/recv/send architectures.
- Utilizes `io_uring` registered buffers, multishot accept, and strictly registered/fixed file descriptors for all internal operations.

**Zero-Copy Pipeline:**
- Large payloads are routed via `SPLICE_F_MOVE`, pushing data directly from the client network socket to the backend socket through an internal kernel pipe ring buffer.

**Persistent Connection Pooling:**
- **BackendPool:**  Maintains persistent, keep-alive TCP connections across multiple backend servers, amortizing the cost of the TCP 3-way handshake to zero for recurring traffic.
- **PipePool:** Pre-allocates thousands of kernel pipes at startup, checking them out atomically during requests to prevent file-descriptor starvation and syscall overhead.

**Multiple Balancing Algorithms**:
- Round Robin
- Least Connections
(Implemented via RouteStrategy.cpp)

**Modern C++ Stack**: Built using io_uring, C++20.

### 🛠️ Prerequisites
Because this project relies on Linux kernel features, you must be running Linux. A kernel version of 5.19+ is highly recommended to support advanced io_uring flags (like Multishot Accept).
- A C++ compiler supporting C++20 (e.g., g++ 11+ or clang++ 14+)
- CMake (version 3.10 or newer)
- liburing (liburing-dev) - Core asynchronous I/O engine
- TOML++ (libtomlplusplus-dev) - Configuration parsing
- EFSW (libefsw-dev) - File watching for hot-reloads

### Example for Ubuntu/Debian
#### Note: You may need to build some of these from source or use a package manager like vcpkg
```bash
sudo apt-get update
sudo apt-get install build-essential g++ cmake liburing-dev libssl-dev
```
### Install other dependencies as required (e.g., vcpkg install crow cpr tomlplusplus efsw)


🚀 Building the Project

This project uses CMake for building along with fetchContent.

##### Clone the repository:

```bash
git clone https://github.com/yourusername/cpp-load-balancer.git
cd cpp-load-balancer

# Create a build directory
cmake -B build

# Compile the project
cmake --build build -j$(nproc)
```


##### This will create the load_balancer executable inside the build/ directory.

### 🏃 Usage
To achieve maximum performance, ensure you raise your OS file descriptor limits before running (e.g., ulimit -n 65535).
Run the application:
```bash
./build/load_balancer
```

The application will start, load the config.toml, and begin monitoring it for changes.

### Example config.toml
``` toml
[load_balancer]
listen_port = 8080
algorithm = "RoundRobin"
queue_depth = 1024       # io_uring submission queue size
max_connections = 10000

[health_check]
path = "/health"
interval = 10

[[servers]]
name = "Server 1"
url = "http://127.0.0.1:8081"
connections = 200

[[servers]]
name = "Server 2"
url = "http://127.0.0.1:8082"
connections = 200
```

### 📂 Project Structure
```
.
.
├── src/
│   ├── main.cpp                # Application entry point
│   ├── IoUringEngine.cpp       # The core event loop and state machine
│   ├── BackendPool.cpp         # Manages persistent Go backend sockets
│   ├── PipePool.cpp            # Pre-allocates kernel pipes for splice()
│   ├── RouteStrategy.cpp       # Balancing algorithms (RoundRobin/LeastConn)
│   ├── tomlParser.cpp          # Config loader
│   ├── healthChecker.cpp       # Async backend monitoring
│   └── HotReloader.cpp         # Live config file watcher
├── include/
│   └── *.hpp                   # Header files
├── CMakeLists.txt
└── README.md
```

### 📄 License
This project is licensed under the MIT License - see the LICENSE file for details.
