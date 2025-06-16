#pragma once

#include <cstddef>
#include <stdexcept>
#include <cstdint>
#include <mutex>
class MemoryArena {
private:
    char* const base_ptr;    
    char* current_ptr;       
    const size_t total_size; 
    mutable std::mutex arena_mutex;
public:
    explicit MemoryArena(size_t size);
    ~MemoryArena();
    
    
    MemoryArena(const MemoryArena&) = delete;
    MemoryArena& operator=(const MemoryArena&) = delete;

    template<typename T>
    T* allocate();
    template<typename T>  
    void deallocate(T*);
    template<typename T>
    T* allocate_array(size_t count);
    template<typename T>  
    void deallocate_array(T* array, size_t count);
    
    void reset();
    size_t remaining() const;
    char* get_alignment(size_t alignment);
};
inline MemoryArena::MemoryArena(size_t size) 
    : base_ptr(new char[size]),    
      current_ptr(base_ptr),       
      total_size(size)            
{
    
    if (!base_ptr) {
        throw std::bad_alloc();  
    }
}

inline MemoryArena::~MemoryArena() 
{
    delete[] base_ptr;  
}

inline void MemoryArena::reset(){
    std::lock_guard<std::mutex> lock(arena_mutex);
    current_ptr = base_ptr;
}

inline size_t MemoryArena::remaining() const {
    std::lock_guard<std::mutex> lock(arena_mutex);
    return total_size - (current_ptr - base_ptr);
}

inline char* MemoryArena::get_alignment(size_t alignment)
{
    uintptr_t addr = reinterpret_cast<uintptr_t>(current_ptr);
    uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);

    char* aligned_ptr = reinterpret_cast<char*>(aligned_addr);
    return aligned_ptr;
} 

template<typename T>
T* MemoryArena::allocate() 
{
    std::lock_guard<std::mutex> lock(arena_mutex);
    constexpr size_t alignment = alignof(T);
    char* aligned_ptr = get_alignment(alignment);
    if (aligned_ptr + sizeof(T) > base_ptr + total_size) {
        return nullptr;
    }
    current_ptr = aligned_ptr + sizeof(T);
    T* new_object = new(aligned_ptr) T();
    return new_object;
}

template<typename T>
void MemoryArena::deallocate(T* object) 
{
    std::lock_guard<std::mutex> lock(arena_mutex);
    if (current_ptr - sizeof(T) >= base_ptr) {
        //deconstruct
        object->~T();
        current_ptr -= sizeof(T);
    }  
}

template<typename T>
T* MemoryArena::allocate_array(size_t count) 
{
    std::lock_guard<std::mutex> lock(arena_mutex);
    
    if (count == 0) return nullptr;
    if (count > SIZE_MAX / sizeof(T)) return nullptr;

    size_t array_size = count * sizeof(T);
    constexpr size_t alignment = alignof(T);
    char* aligned_ptr = get_alignment(alignment);

    if (aligned_ptr + array_size > base_ptr + total_size) {  
        return nullptr;
    }

    T* array_start = reinterpret_cast<T*>(aligned_ptr);
    for (size_t i = 0; i < count; ++i) {
        new(array_start + i) T();  
    }

    current_ptr = aligned_ptr + array_size;
    return array_start;
}
template<typename T>
void MemoryArena::deallocate_array(T* array, size_t count)
{
    std::lock_guard<std::mutex> lock(arena_mutex);
    if (count == 0) return;
    size_t array_size = count * sizeof(T);
       
    for (size_t i = count; i > 0; --i) {
        (array + i - 1)->~T();
    }
    if (current_ptr - array_size >= base_ptr) {
        current_ptr -= array_size;
    }
}