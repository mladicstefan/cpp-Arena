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
    
    void reset();
    size_t remaining() const;
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

template<typename T>
T* MemoryArena::allocate() 
{
    std::lock_guard<std::mutex> lock(arena_mutex);
    constexpr size_t alignment = alignof(T);
    
    uintptr_t addr = reinterpret_cast<uintptr_t>(current_ptr);
    uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);

    char* aligned_ptr = reinterpret_cast<char*>(aligned_addr);
    if (aligned_ptr + sizeof(T) > base_ptr + total_size) {
        return nullptr;
    }
    
    current_ptr = aligned_ptr + sizeof(T);
    //return reinterpret_cast<T*>(aligned_ptr);
    T* new_object = new(aligned_ptr) T();
    return new_object;
}

template<typename T>
void MemoryArena::deallocate(T* object) 
{
    std::lock_guard<std::mutex> lock(arena_mutex);
    if (current_ptr - sizeof(T) >= base_ptr) {
        object->~T();
        current_ptr -= sizeof(T);
    }  
}