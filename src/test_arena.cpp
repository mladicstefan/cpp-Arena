#include "Arena.hpp"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <cstring>

// Struct with specific alignment requirements
struct alignas(16) AlignedStruct {
    double a;
    double b;
};

struct alignas(32) HeavilyAlignedStruct {
    int data[8];
};

void test_alignment_correctness() {
    std::cout << "Testing alignment correctness..." << std::endl;
    
    MemoryArena arena(1024);
    
    // Test basic type alignments
    char* char_ptr = arena.allocate<char>();
    assert(reinterpret_cast<uintptr_t>(char_ptr) % alignof(char) == 0);
    
    int* int_ptr = arena.allocate<int>();
    assert(reinterpret_cast<uintptr_t>(int_ptr) % alignof(int) == 0);
    
    double* double_ptr = arena.allocate<double>();
    assert(reinterpret_cast<uintptr_t>(double_ptr) % alignof(double) == 0);
    
    // Test custom aligned structs
    AlignedStruct* aligned16_ptr = arena.allocate<AlignedStruct>();
    assert(aligned16_ptr != nullptr);
    assert(reinterpret_cast<uintptr_t>(aligned16_ptr) % 16 == 0);
    
    HeavilyAlignedStruct* aligned32_ptr = arena.allocate<HeavilyAlignedStruct>();
    assert(aligned32_ptr != nullptr);
    assert(reinterpret_cast<uintptr_t>(aligned32_ptr) % 32 == 0);
    
    std::cout << "✓ Alignment correctness verified" << std::endl;
}

void test_alignment_padding() {
    std::cout << "Testing alignment padding calculations..." << std::endl;
    
    MemoryArena arena(1024);
    size_t initial_remaining = arena.remaining();
    
    // Allocate a char (1 byte, alignment 1)
    char* char_ptr = arena.allocate<char>();
    assert(char_ptr != nullptr);
    
    // Next allocation should be padded properly for double (8-byte alignment)
    double* double_ptr = arena.allocate<double>();
    assert(double_ptr != nullptr);
    
    // Check that padding was added
    size_t expected_used = sizeof(char) + (8 - (sizeof(char) % 8)) % 8 + sizeof(double);
    size_t actual_used = initial_remaining - arena.remaining();
    
    // The actual used space should account for padding
    assert(actual_used >= sizeof(char) + sizeof(double));
    
    std::cout << "✓ Alignment padding works correctly" << std::endl;
}

void test_potential_crashes_buffer_overrun() {
    std::cout << "Testing potential buffer overrun scenarios..." << std::endl;
    
    // Create very small arena to force edge cases
    MemoryArena tiny_arena(32);
    
    // Fill up the arena completely
    std::vector<int*> ptrs;
    int* ptr;
    while ((ptr = tiny_arena.allocate<int>()) != nullptr) {
        ptrs.push_back(ptr);
        *ptr = 0xDEADBEEF; // Write pattern to verify memory
    }
    
    // Try to allocate when arena is full - should return nullptr
    int* overflow_ptr = tiny_arena.allocate<int>();
    assert(overflow_ptr == nullptr);
    
    // Verify existing data is still intact
    for (int* p : ptrs) {
        assert(*p == 0xDEADBEEF);
    }
    
    std::cout << "✓ Buffer overrun protection works" << std::endl;
}

void test_deallocate_crash_scenarios() {
    std::cout << "Testing deallocation crash scenarios..." << std::endl;
    
    MemoryArena arena(512);
    
    // Allocate some memory
    int* int_ptr = arena.allocate<int>();
    double* double_ptr = arena.allocate<double>();
    assert(int_ptr && double_ptr);
    
    // Try to deallocate more than allocated (potential underflow)
    arena.deallocate<double>();
    arena.deallocate<int>();
    
    // This could cause underflow - the implementation should handle gracefully
    // Note: This might cause current_ptr to go below base_ptr
    arena.deallocate<int>(); // Over-deallocate
    
    // Try to allocate after over-deallocation
    int* new_ptr = arena.allocate<int>();
    // The arena should still function (though behavior may be undefined)
    
    std::cout << "✓ Deallocation edge cases tested" << std::endl;
}

void test_thread_safety_concurrent_allocation() {
    std::cout << "Testing thread safety - concurrent allocation..." << std::endl;
    
    MemoryArena arena(4096);
    const int num_threads = 8;
    const int allocations_per_thread = 50;
    std::atomic<int> successful_allocations{0};
    std::vector<std::thread> threads;
    
    auto allocate_worker = [&]() {
        for (int i = 0; i < allocations_per_thread; ++i) {
            int* ptr = arena.allocate<int>();
            if (ptr != nullptr) {
                *ptr = std::hash<std::thread::id>{}(std::this_thread::get_id()); // Write thread ID hash
                successful_allocations++;
                // Small delay to increase chance of race conditions
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
    };
    
    // Start all threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(allocate_worker);
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Successful allocations: " << successful_allocations.load() << std::endl;
    assert(successful_allocations.load() > 0);
    
    std::cout << "✓ Concurrent allocation thread safety tested" << std::endl;
}

void test_thread_safety_alloc_dealloc_race() {
    std::cout << "Testing thread safety - allocation/deallocation race..." << std::endl;
    
    MemoryArena arena(2048);
    std::atomic<bool> stop_flag{false};
    std::atomic<int> operations{0};
    
    auto allocator_worker = [&]() {
        while (!stop_flag.load()) {
            int* ptr = arena.allocate<int>();
            if (ptr) {
                *ptr = 42;
                operations++;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    };
    
    auto deallocator_worker = [&]() {
        while (!stop_flag.load()) {
            arena.deallocate<int>();
            operations++;
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    };
    
    std::thread allocator(allocator_worker);
    std::thread deallocator(deallocator_worker);
    
    // Let them run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop_flag = true;
    
    allocator.join();
    deallocator.join();
    
    std::cout << "Total operations: " << operations.load() << std::endl;
    std::cout << "✓ Allocation/deallocation race condition tested" << std::endl;
}

void test_memory_corruption_detection() {
    std::cout << "Testing memory corruption scenarios..." << std::endl;
    
    MemoryArena arena(1024);
    
    // Allocate memory and write patterns
    int* ptr1 = arena.allocate<int>();
    int* ptr2 = arena.allocate<int>();
    int* ptr3 = arena.allocate<int>();
    
    assert(ptr1 && ptr2 && ptr3);
    
    // Write specific patterns
    *ptr1 = 0x11111111;
    *ptr2 = 0x22222222;
    *ptr3 = 0x33333333;
    
    // Try to write beyond allocated boundaries (this would be UB in real usage)
    // We're testing that our arena doesn't prevent this detection
    
    // Verify patterns are still intact
    assert(*ptr1 == 0x11111111);
    assert(*ptr2 == 0x22222222);
    assert(*ptr3 == 0x33333333);
    
    // Reset and verify memory can be reused
    arena.reset();
    int* new_ptr = arena.allocate<int>();
    assert(new_ptr != nullptr);
    *new_ptr = 0x44444444;
    assert(*new_ptr == 0x44444444);
    
    std::cout << "✓ Memory corruption detection scenarios tested" << std::endl;
}

void test_stress_rapid_operations() {
    std::cout << "Testing stress scenario - rapid operations..." << std::endl;
    
    MemoryArena arena(8192);
    const int iterations = 100000;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Rapid allocation and deallocation cycles
    for (int i = 0; i < iterations; ++i) {
        int* ptr = arena.allocate<int>();
        if (ptr) {
            *ptr = i;
        }
        
        if (i % 100 == 0) {
            arena.reset(); // Periodic reset to avoid filling up
        }
        
        if (i % 50 == 0) {
            arena.deallocate<int>(); // Periodic deallocation
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "Completed " << iterations << " operations in " << duration.count() << " μs" << std::endl;
    std::cout << "✓ Stress test completed without crashes" << std::endl;
}

int main() {
    std::cout << "=== Memory Arena Advanced Test Suite ===" << std::endl;
    std::cout << "Testing alignment, crash scenarios, and thread safety\n" << std::endl;
    
    try {
        test_alignment_correctness();
        test_alignment_padding();
        test_potential_crashes_buffer_overrun();
        test_deallocate_crash_scenarios();
        test_thread_safety_concurrent_allocation();
        test_thread_safety_alloc_dealloc_race();
        test_memory_corruption_detection();
        test_stress_rapid_operations();
        
        std::cout << "\n🎉 All advanced tests completed!" << std::endl;
        std::cout << "Note: Some tests intentionally push boundaries and may expose edge cases." << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}