# 2Q Cache Implementation in C++

A robust, highly efficient implementation of the **2Q Caching Algorithm** in C++. This project is designed with a focus on performance, system-level memory management, and clean architecture.

## About the Algorithm & Architecture
The 2Q algorithm behaves like LRU but prevents "cache pollution" by using multiple queues. It filters out pages that are accessed only once, promoting only frequently accessed pages to the main hot cache.

### Key Architectural Feature: Circular Buffer
To maximize performance and avoid costly dynamic memory allocations, the `IN` queue (which stores initially requested pages) is implemented using a custom **Circular Buffer (Ring Buffer)**. This guarantees strict `O(1)` time complexity for push and pop operations while maintaining a fixed memory footprint.

## Features
* **High Performance:** Written in C++ with strict memory safety in mind.
* **Custom Ring Buffer:** Hardware-friendly memory layout for the `IN` queue.
* **Extensive Testing:** Covered by both unit tests (Google Test) and end-to-end Python test scripts.
* **CMake Build System:** Clean and modular build pipeline.

## Prerequisites
To build and run this project, you will need:
* A C++ compiler supporting C++23 (e.g., GCC 13+, Clang 16+)
* [CMake](https://cmake.org/) (version 3.13 or higher)
* Git

## Getting Started

### 1. Clone the repository
```bash
git clone [https://github.com/your-username/cache_2Q.git](https://github.com/your-username/cache_2Q.git)
cd cache_2Q
2. Build the project

The project uses CMake for building. Run the following commands from the root directory:
Bash

cmake -B build
cmake --build build

Running Tests

The project includes comprehensive testing to ensure cache logic correctness.
Unit Tests (Google Test)

The C++ unit tests check the internal data structures (including the Ring Buffer) and the logic of the 2Q cache. To run them:
Bash

cd build/Tests
./unit_tests

End-to-End Tests (Python)

The project also provides an E2E testing script to simulate real-world workloads and check the overall caching efficiency:

python3 Tests/e2e_tests/run.py