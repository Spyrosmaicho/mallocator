#ifndef MY_MALLOC_H
#define MY_MALLOC_H

typedef struct Block Block;
typedef struct Footer footer;

#define ALIGNMENT 16
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

#define MMAP_THRESHOLD (4096)
#define IS_MMAP(size) ((size) >= MMAP_THRESHOLD)

//Function to allocate memory
void* my_malloc(size_t size);
//Function to free allocated memory
void my_free(void* ptr);
// Function to print memory statistics
void print_memory_stats();

#endif // MY_MALLOC_H