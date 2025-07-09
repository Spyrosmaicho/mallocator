#ifndef MY_MALLOC_H
#define MY_MALLOC_H

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

#define MMAP_THRESHOLD (4096)  
#define IS_MMAP(size) ((size) >= MMAP_THRESHOLD)

#include <stddef.h>

//Function to allocate memory
void* my_malloc(size_t size);
//Function to make my calloc
void *my_calloc(size_t nmemb, size_t size);
//Function to make my realloc
void *my_realloc(void *ptr, size_t size);
//Function to free allocated memory
void my_free(void* ptr);
// Function to print memory statistics
void print_memory_stats();

#endif // MY_MALLOC_H