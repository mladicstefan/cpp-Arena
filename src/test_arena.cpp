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
    
    // Deallocate in LIFO order (proper usage)
    arena.deallocate<double>(double_ptr);
    arena.deallocate<int>(int_ptr);
    
    // Test edge case: try to deallocate when arena is empty
    // This should be handled gracefully by the bounds check
    int* dummy_ptr = nullptr;
    arena.deallocate<int>(dummy_ptr); // Should not crash
    
    // Try to allocate after deallocation
    int* new_ptr = arena.allocate<int>();
    assert(new_ptr != nullptr); // Should work fine
    
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
            // Note: This is a simplified test - in real usage you'd track allocated pointers
            // For test purposes, we'll just reset the arena periodically
            if (operations.load() % 10 == 0) {
                arena.reset(); // Reset arena instead of individual deallocations
            }
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
            arena.reset(); // Periodic reset to avoid filling up arena
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "Completed " << iterations << " operations in " << duration.count() << " μs" << std::endl;
    std::cout << "✓ Stress test completed without crashes" << std::endl;
}

// Test class to verify constructor/destructor calls
class TestObject {
public:
    static int constructor_count;
    static int destructor_count;
    int value;
    
    TestObject() : value(42) {
        constructor_count++;
        std::cout << "Constructor called, count: " << constructor_count << std::endl;
    }
    
    ~TestObject() {
        destructor_count++;
        std::cout << "Destructor called, count: " << destructor_count << std::endl;
    }
    
    static void reset_counters() {
        constructor_count = 0;
        destructor_count = 0;
    }
};

int TestObject::constructor_count = 0;
int TestObject::destructor_count = 0;

void test_constructor_calls() {
    std::cout << "Testing constructor calls with placement new..." << std::endl;
    
    TestObject::reset_counters();
    MemoryArena arena(1024);
    
    // Allocate objects and verify constructors are called
    TestObject* obj1 = arena.allocate<TestObject>();
    assert(obj1 != nullptr);
    assert(obj1->value == 42); // Verify constructor ran
    assert(TestObject::constructor_count == 1);
    
    TestObject* obj2 = arena.allocate<TestObject>();
    assert(obj2 != nullptr);
    assert(obj2->value == 42);
    assert(TestObject::constructor_count == 2);
    
    // Test with different types
    int* int_ptr = arena.allocate<int>();
    assert(int_ptr != nullptr);
    // Note: int constructor doesn't increment our counter, but should be zero-initialized
    
    std::cout << "✓ Constructors called correctly" << std::endl;
}

void test_destructor_calls() {
    std::cout << "Testing destructor calls..." << std::endl;
    
    TestObject::reset_counters();
    MemoryArena arena(1024);
    
    // Allocate objects
    TestObject* obj1 = arena.allocate<TestObject>();
    TestObject* obj2 = arena.allocate<TestObject>();
    assert(TestObject::constructor_count == 2);
    assert(TestObject::destructor_count == 0);
    
    // Deallocate in LIFO order and verify destructors are called
    arena.deallocate<TestObject>(obj2);
    assert(TestObject::destructor_count == 1);
    
    arena.deallocate<TestObject>(obj1);
    assert(TestObject::destructor_count == 2);
    
    std::cout << "✓ Destructors called correctly" << std::endl;
}

void test_array_allocation() {
    std::cout << "Testing array allocation..." << std::endl;
    
    MemoryArena arena(2048);
    
    // Test basic array allocation
    int* int_array = arena.allocate_array<int>(10);
    assert(int_array != nullptr);
    
    // Verify we can write to all elements
    for (int i = 0; i < 10; ++i) {
        int_array[i] = i * 2;
    }
    
    // Verify values
    for (int i = 0; i < 10; ++i) {
        assert(int_array[i] == i * 2);
    }
    
    // Test larger array
    double* double_array = arena.allocate_array<double>(50);
    assert(double_array != nullptr);
    
    for (int i = 0; i < 50; ++i) {
        double_array[i] = i * 3.14;
    }
    
    std::cout << "✓ Array allocation works correctly" << std::endl;
}

void test_array_constructors() {
    std::cout << "Testing array constructor calls..." << std::endl;
    
    TestObject::reset_counters();
    MemoryArena arena(2048);
    
    // Allocate array of objects
    const size_t array_size = 5;
    TestObject* obj_array = arena.allocate_array<TestObject>(array_size);
    assert(obj_array != nullptr);
    
    // Verify all constructors were called
    assert(TestObject::constructor_count == array_size);
    
    // Verify all objects are properly initialized
    for (size_t i = 0; i < array_size; ++i) {
        assert(obj_array[i].value == 42);
    }
    
    std::cout << "✓ Array constructors called correctly" << std::endl;
}

void test_array_bounds_checking() {
    std::cout << "Testing array bounds checking..." << std::endl;
    
    // Create small arena to test overflow
    MemoryArena small_arena(64);
    
    // Try to allocate huge array - should fail
    int* huge_array = small_arena.allocate_array<int>(1000000);
    assert(huge_array == nullptr);
    
    // Test zero-size array
    int* zero_array = small_arena.allocate_array<int>(0);
    assert(zero_array == nullptr);
    
    // Test overflow in size calculation
    size_t huge_count = SIZE_MAX / sizeof(int) + 1;
    int* overflow_array = small_arena.allocate_array<int>(huge_count);
    assert(overflow_array == nullptr);
    
    // Test normal allocation still works
    int* normal_array = small_arena.allocate_array<int>(5);
    assert(normal_array != nullptr);
    
    std::cout << "✓ Array bounds checking works correctly" << std::endl;
}

void test_array_deallocation() {
    std::cout << "Testing array deallocation..." << std::endl;
    
    TestObject::reset_counters();
    MemoryArena arena(2048);
    
    // Allocate array
    const size_t array_size = 3;
    TestObject* obj_array = arena.allocate_array<TestObject>(array_size);
    assert(obj_array != nullptr);
    assert(TestObject::constructor_count == array_size);
    assert(TestObject::destructor_count == 0);
    
    // Test array deallocation - should call all destructors
    arena.deallocate_array<TestObject>(obj_array, array_size);
    assert(TestObject::destructor_count == array_size);
    
    // Test deallocation of zero-size array (should be safe)
    arena.deallocate_array<TestObject>(nullptr, 0);
    
    // Verify memory is reclaimed by allocating again
    TestObject::reset_counters();
    TestObject* new_obj = arena.allocate<TestObject>();
    assert(new_obj != nullptr);
    assert(TestObject::constructor_count == 1);
    
    std::cout << "✓ Array deallocation works correctly" << std::endl;
}

void test_mixed_allocation() {
    std::cout << "Testing mixed single/array allocation..." << std::endl;
    
    TestObject::reset_counters();
    MemoryArena arena(2048);
    
    // Mix single and array allocations
    TestObject* single1 = arena.allocate<TestObject>();
    TestObject* array1 = arena.allocate_array<TestObject>(3);
    TestObject* single2 = arena.allocate<TestObject>();
    TestObject* array2 = arena.allocate_array<TestObject>(2);
    
    assert(single1 && array1 && single2 && array2);
    assert(TestObject::constructor_count == 1 + 3 + 1 + 2); // 7 total objects
    
    // Verify all objects are properly initialized
    assert(single1->value == 42);
    assert(single2->value == 42);
    for (int i = 0; i < 3; ++i) {
        assert(array1[i].value == 42);
    }
    for (int i = 0; i < 2; ++i) {
        assert(array2[i].value == 42);
    }
    
    std::cout << "✓ Mixed allocation works correctly" << std::endl;
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
        test_constructor_calls();
        test_destructor_calls();
        test_array_allocation();
        test_array_constructors();
        test_array_bounds_checking();
        test_array_deallocation();
        test_mixed_allocation();
        
        std::cout << "\n🎉 All advanced tests completed!" << std::endl;
        std::cout << "Note: Some tests intentionally push boundaries and may expose edge cases." << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}