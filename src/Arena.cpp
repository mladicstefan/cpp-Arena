
#include "Arena.hpp"

MemoryArena::MemoryArena(size_t size) 
    : base_ptr(new char[size]),    
      current_ptr(base_ptr),       
      total_size(size)            
{
    
    if (!base_ptr) {
        throw std::bad_alloc();  
    }
}

MemoryArena::~MemoryArena() 
{
    delete[] base_ptr;  
}

void MemoryArena::reset(){
    current_ptr = base_ptr;
}

size_t MemoryArena::remaining() const {
    return total_size - (current_ptr - base_ptr);
}
