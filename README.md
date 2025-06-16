# cpp::Arena

A simple, stack-based memory arena for C++ built for personal use and learning. Features automatic memory alignment, thread safety, and efficient allocation patterns.

## Features
- **Automatic Alignment**: Properly aligns allocations for any type, including custom aligned structs
- **Thread Safe**: All operations are protected by mutexes for concurrent use
- **Stack-based Deallocation**: LIFO deallocation pattern for efficient memory management
- **Header-only**: No compilation required, just include the header

## Usage
### Basic Usage
```cpp
#include "Arena.hpp"

int main() {
    // Create 1KB arena
    MemoryArena arena(1024);
    
    // Allocate different types
    int* my_int = arena.allocate<int>();
    double* my_double = arena.allocate<double>();
    
    *my_int = 42;
    *my_double = 3.14159;
    
    // Deallocate in reverse order (LIFO)
    arena.deallocate<double>();
    arena.deallocate<int>();
    
    // Or reset everything at once
    arena.reset();
    
    return 0;
}
```

### Multithreaded Example
```cpp
#include "Arena.hpp"
#include <thread>
#include <vector>
#include <iostream>

int main() {
    MemoryArena arena(4096);  // 4KB arena
    const int num_threads = 4;
    std::vector<std::thread> threads;
    
    // Worker function for each thread
    auto worker = [&arena](int thread_id) {
        for (int i = 0; i < 10; ++i) {
            // Thread-safe allocation
            int* ptr = arena.allocate<int>();
            if (ptr) {
                *ptr = thread_id * 100 + i;
                std::cout << "Thread " << thread_id 
                          << " allocated: " << *ptr << std::endl;
            }
            
            // Small delay to demonstrate concurrency
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    };
    
    // Start all threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker, i);
    }
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Remaining space: " << arena.remaining() << " bytes" << std::endl;
    
    return 0;
}
```
## API
### Core Operations
```cpp
MemoryArena arena(size);          // Create arena with given size
T* ptr = arena.allocate<T>();     // Allocate space for type T (thread-safe)
arena.deallocate<T>();            // Deallocate last allocation (thread-safe)
arena.reset();                    // Reset to empty state (thread-safe)
size_t space = arena.remaining(); // Get remaining bytes (thread-safe)
```

### Alignment Guarantees
- All allocations are automatically aligned to the requirements of type `T`
- Uses `alignof(T)` to determine proper alignment
- Handles custom `alignas` specifications
- Padding is automatically inserted between allocations when needed

### Thread Safety
- All public methods are thread-safe
- Uses `std::mutex` for synchronization
- Safe for concurrent allocation/deallocation from multiple threads
- No data races or memory corruption in multithreaded environments

## Building

Header-only library - no compilation needed. Just include `Arena.hpp`.

### Requirements
- C++17 or later
- Standard library support for `<mutex>`, `<cstdint>`

### Testing
```bash
# Compile and run basic tests
g++ -std=c++17 test_arena.cpp -o test && ./test

# For multithreaded testing, link pthread on some systems
g++ -std=c++17 -pthread test_arena.cpp -o test && ./test
```

## Implementation Details

- **Memory Layout**: Linear allocation from a single contiguous block
- **Alignment**: Uses bit manipulation for efficient alignment calculations
- **Deallocation**: Simple pointer arithmetic (stack-based LIFO only)
- **Thread Safety**: Mutex protection on all operations
- **Error Handling**: Returns `nullptr` on allocation failure, graceful handling of over-deallocation

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
