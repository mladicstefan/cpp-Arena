# cpp::Arena

A simple, stack-based memory arena for C++ built for personal use and learning.

## Usage
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
    
    // Deallocate in reverse order
    arena.deallocate<double>();
    arena.deallocate<int>();
    
    // Or reset everything at once
    arena.reset();
    
    return 0;
}
```

## API
```cpp
MemoryArena arena(size);          // Create arena with given size
T* ptr = arena.allocate<T>();     // Allocate space for type T
arena.deallocate<T>();            // Deallocate last allocation
arena.reset();                    // Reset to empty state
size_t space = arena.remaining(); // Get remaining bytes
```

## Building
Header-only library - no compilation needed. Just include `Arena.hpp`.

## Testing
```bash
g++ -std=c++17 test_arena.cpp -o test && ./test
```

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
