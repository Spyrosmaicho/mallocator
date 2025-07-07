#include <stdbool.h>
#include <stdlib.h>
#include "my_malloc.h"
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

static pthread_mutex_t alloc_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifndef MAP_ANONYMOUS
    #ifdef MAP_ANON
        #define MAP_ANONYMOUS MAP_ANON
    #else
        // Fallback: define MAP_ANONYMOUS to 0 if not available (may not work on all systems)
        #define MAP_ANONYMOUS 0
    #endif
#endif


typedef struct Block {
    size_t size;
    bool free;
    bool is_mmap; // Flag to indicate if the block was allocated using mmap
    struct Block* next;
    struct Block* prev;
} Block;
 
typedef struct Footer
{
    size_t size; //for coleascing
}Footer;

#define BLOCK_SIZE sizeof(struct Block)
#define MIN_BLOCK_SIZE (ALIGN( sizeof(struct Block) + sizeof(struct Footer) + ALIGNMENT))

Block* head = NULL;
Block* tail = NULL;

Block *find_best_fit(size_t size) 
{
    Block* best = NULL;
    Block* current = head;
    while(current)
    {
        if(current->free && current->size >= size)
        {
            if(!best || current->size < best->size) //Find the smallest block that fits
            {
                best = current;
                // Perfect fit found, stop searching
                if (best->size == size) break;
            }
        }
        current = current->next;
    }   
    return best; //Return the best fit block
}

Footer* get_Footer(Block *block)
{
    return (Footer*)((char*)(block +1) + block->size);
}

Block *request_space(size_t size)
{
    void *request;
    Block *block;

    if (IS_MMAP(size)) 
    {    
        request = mmap(NULL, size + sizeof(Block) + sizeof(Footer),PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (request == MAP_FAILED) return NULL;
        
        block = (Block*)request;
        block->size = size;
        block->free = false;
        block->next = NULL;
        block->prev = tail;
        
        block->is_mmap = true;
    }
    else
    {
        size_t page_size = getpagesize();
        size_t request_size = ((size + page_size - 1) / page_size) * page_size;
        
        request = sbrk(request_size);
        if (request == (void*)-1) return NULL;
        
        block = (Block*)request;
        block->size = request_size - sizeof(Block) - sizeof(Footer);
        block->free = false;
        block->next = NULL;
        block->prev = tail;
        block->is_mmap = false;
    }

    Footer *foot = get_Footer(block);
    foot->size = block->size;
    
    // Update global list
    if (!head) head = block;
    if (tail) tail->next = block;
    tail = block;
    
    return block;
}

void split(Block *block,size_t size)
{
    Block * new = (Block*)((char*)(block + 1) + size);
    new->size = block->size - size - sizeof(Block) - sizeof(Footer);
    new->free = true;
    new->next = block->next;
    new->prev = block;

    block->size = size;
    block->next = new;

    Footer *new_Footer = get_Footer(new);
    new_Footer->size = new->size;

    Footer *block_Footer = get_Footer(block);
    block_Footer->size = block->size;

    if (new->next) new->next->prev = new;
    else tail = new;    
}


void* my_malloc(size_t size)
{
    pthread_mutex_lock(&alloc_mutex);

    Block *block;
    if(size <= 0)
    {
        pthread_mutex_unlock(&alloc_mutex);
        return NULL; //Invalid size
    }
    size_t  actual_size = ALIGN(size);
    size_t  total_size = sizeof(Block) + actual_size + sizeof(Footer);

    block = find_best_fit(total_size);
    if(!block)
    {
        block = request_space(total_size);
        if(!block)  
        {
            pthread_mutex_unlock(&alloc_mutex);
            return NULL;
        }
    }
    else
    {
        if(block->size >= total_size + MIN_BLOCK_SIZE) split(block, total_size);
        block->free = false; //Mark the block as used
    }
    
    pthread_mutex_unlock(&alloc_mutex);
    return (void*)(block + 1); //Return a posize_ter to the memory after the block header

}

void coalesce_blocks(Block *block)
{
   pthread_mutex_lock(&alloc_mutex);
    if(!block) 
    {
        pthread_mutex_unlock(&alloc_mutex);     
        return; //Invalid block
    }

    if(block->next && block->next->free)
    {
        block->size += block->next->size + sizeof(Block) + sizeof(Footer);

        Footer *foot = get_Footer(block);
        foot->size = block->size;

        block->next = block->next->next; //Remove the next block from the list
        if(block->next) block->next->prev = block; //Update the previous posize_ter
        else tail = block; //If the next block was the tail, update the tail posize_ter
    }

    if (block->prev && block->prev->free) 
    {
        block->prev->size += sizeof(Block) + block->size + sizeof(Footer);
        
        //Update Footer
        Footer* foot = get_Footer(block->prev);
        foot->size = block->prev->size;
        
        //Update next posize_ter
        block->prev->next = block->next;
        
        if (block->next) block->next->prev = block->prev;
        else tail = block->prev;
    }
    pthread_mutex_unlock(&alloc_mutex);
}


Block *get_block_ptr(void *ptr) 
{
  return (Block*)ptr - 1;
}

void my_free(void* ptr)
{
    pthread_mutex_lock(&alloc_mutex);
    if(!ptr) 
    {
        pthread_mutex_unlock(&alloc_mutex);
        return; //Invalid posize_ter
    }

    Block *block_ptr = get_block_ptr(ptr);
    if(block_ptr->free) 
    {
        pthread_mutex_unlock(&alloc_mutex);
        return; //Block already free
    }
    

    if(block_ptr->is_mmap) 
    {
        munmap(block_ptr, block_ptr->size + sizeof(Block) + sizeof(Footer));
        if (block_ptr->prev) block_ptr->prev->next = block_ptr->next;
        if (block_ptr->next) block_ptr->next->prev = block_ptr->prev;
        if (head == block_ptr) head = block_ptr->next;
        if (tail == block_ptr) tail = block_ptr->prev;
    }
    else
    {
        block_ptr->free = true; //Mark the block as free
        coalesce_blocks(block_ptr);
    }
    pthread_mutex_unlock(&alloc_mutex);
}

// Function to prsize_t memory statistics
void print_memory_stats() 
{
    size_t total = 0, used = 0;
    size_t blocks = 0, mmap_blocks = 0;
    
    Block* curr = head;
    while (curr) {
        total += curr->size + sizeof(Block) + sizeof(Footer);
        blocks++;
        if (curr->is_mmap) mmap_blocks++;
        if (!curr->free) used += curr->size;
        curr = curr->next;
    }
    
    printf("Memory Stats:\n");
    printf("Total: %zu bytes\n", total);
    printf("Used: %zu bytes\n", used);
    printf("Blocks: %ld (%ld mmap)\n", blocks, mmap_blocks);
}