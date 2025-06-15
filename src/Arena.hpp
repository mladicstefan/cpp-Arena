#pragma once

#include <cstddef>
#include <stdexcept>

class MemoryArena {
private:
    char* const base_ptr;    
    char* current_ptr;       
    const size_t total_size; 
public:
    explicit MemoryArena(size_t size);
    ~MemoryArena();
    
    
    MemoryArena(const MemoryArena&) = delete;
    MemoryArena& operator=(const MemoryArena&) = delete;
    
    template<typename T>
    T* allocate();
    
    template<typename T>  
    void deallocate();
    
    void reset();
    size_t remaining() const;
};

template<typename T>
T* MemoryArena::allocate() {

    size_t needed = sizeof(T); 
    if (current_ptr + needed > base_ptr + total_size) {
        return nullptr;
    }
    
    T* result = reinterpret_cast<T*>(current_ptr);
    current_ptr += needed;  
    
    return result;
}

template<typename T>
void MemoryArena::deallocate() {
    current_ptr -= sizeof(T);  
}

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
    current_ptr = base_ptr;
}

inline size_t MemoryArena::remaining() const {
    return total_size - (current_ptr - base_ptr);
}
