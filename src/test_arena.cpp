#include "Arena.hpp"
#include <iostream>
#include <cassert>

void test_basic_allocation() {
    std::cout << "Testing basic allocation..." << std::endl;
    
    MemoryArena arena(1024);
    
    // Test single allocations
    int* int_ptr = arena.allocate<int>();
    assert(int_ptr != nullptr);
    *int_ptr = 42;
    assert(*int_ptr == 42);
    
    double* double_ptr = arena.allocate<double>();
    assert(double_ptr != nullptr);
    *double_ptr = 3.14159;
    assert(*double_ptr == 3.14159);
    
    // Test remaining space calculation
    size_t expected_remaining = 1024 - sizeof(int) - sizeof(double);
    assert(arena.remaining() == expected_remaining);
    
    std::cout << "✓ Basic allocation works" << std::endl;
}

void test_out_of_memory() {
    std::cout << "Testing out of memory conditions..." << std::endl;
    
    // Small arena - only 8 bytes
    MemoryArena small_arena(8);
    
    // First allocation should work (4 bytes for int)
    int* ptr1 = small_arena.allocate<int>();
    assert(ptr1 != nullptr);
    assert(small_arena.remaining() == 4);
    
    // Second allocation should work (4 more bytes)
    int* ptr2 = small_arena.allocate<int>();
    assert(ptr2 != nullptr);
    assert(small_arena.remaining() == 0);
    
    // Third allocation should fail (no space left)
    int* ptr3 = small_arena.allocate<int>();
    assert(ptr3 == nullptr);
    
    std::cout << "✓ Out of memory handling works" << std::endl;
}

void test_stack_deallocation() {
    std::cout << "Testing stack deallocation (LIFO)..." << std::endl;
    
    MemoryArena arena(100);
    
    // Allocate in order: int, double, char
    arena.allocate<int>();    // 4 bytes
    arena.allocate<double>(); // 8 bytes  
    arena.allocate<char>();   // 1 byte
    
    size_t used_space = sizeof(int) + sizeof(double) + sizeof(char);
    assert(arena.remaining() == 100 - used_space);
    
    // Deallocate in reverse order (stack discipline)
    arena.deallocate<char>();   // Remove last allocated
    arena.deallocate<double>(); // Remove second-to-last
    arena.deallocate<int>();    // Remove first allocated
    
    // Should be back to original state
    assert(arena.remaining() == 100);
    
    std::cout << "✓ Stack deallocation works" << std::endl;
}

void test_reset_functionality() {
    std::cout << "Testing arena reset..." << std::endl;
    
    MemoryArena arena(512);
    
    // Allocate several objects
    arena.allocate<int>();
    arena.allocate<double>();
    arena.allocate<char>();
    
    // Arena should have less space
    assert(arena.remaining() < 512);
    
    // Reset should restore full capacity
    arena.reset();
    assert(arena.remaining() == 512);
    
    // Should be able to allocate again after reset
    int* new_ptr = arena.allocate<int>();
    assert(new_ptr != nullptr);
    
    std::cout << "✓ Reset functionality works" << std::endl;
}

void test_different_types() {
    std::cout << "Testing different data types..." << std::endl;
    
    MemoryArena arena(1024);
    
    // Test various standard types
    char* c = arena.allocate<char>();        // 1 byte
    short* s = arena.allocate<short>();      // 2 bytes  
    int* i = arena.allocate<int>();          // 4 bytes
    double* d = arena.allocate<double>();    // 8 bytes
    
    // All should succeed
    assert(c != nullptr && s != nullptr && i != nullptr && d != nullptr);
    
    // Test they can be written to and read from
    *c = 'A';
    *s = 100;
    *i = 42;
    *d = 3.14;
    
    assert(*c == 'A');
    assert(*s == 100);
    assert(*i == 42);
    assert(*d == 3.14);
    
    std::cout << "✓ Different data types work correctly" << std::endl;
}

int main() {
    std::cout << "=== Memory Arena Test Suite ===" << std::endl;
    
    try {
        test_basic_allocation();
        test_out_of_memory();
        test_stack_deallocation();
        test_reset_functionality();
        test_different_types();
        
        std::cout << "\n🎉 All tests passed! Your arena is working correctly!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}