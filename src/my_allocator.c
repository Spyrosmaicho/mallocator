#include <stdbool.h>
#include <stdlib.h>
#include "my_allocator.h"
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

static pthread_mutex_t alloc_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifndef MAP_ANONYMOUS
    #ifdef MAP_ANON
        #define MAP_ANONYMOUS MAP_ANON
    #else
        // Fallback: define MAP_ANONYMOUS to 0 if not available (may not work on all systems)
        #define MAP_ANONYMOUS 0
    #endif
#endif

#define FREED_MAGIC 0xDEADBEEFDEADBEEF
#define ALLOC_MAGIC 0xBADC0DEDEAD1234

typedef struct Block {
    size_t size;
    size_t magic;
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
    while(current) //Iterate through the linked list of blocks
    {
        if(current->magic != ALLOC_MAGIC && current->magic != FREED_MAGIC) 
        {
            fprintf(stderr, "Corrupted block detected at %p\n", (void*)current);
            return NULL;
        }
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
    if(!block) return NULL;


    if(block->is_mmap && block->magic != ALLOC_MAGIC && block->magic != FREED_MAGIC) return NULL;

    return (Footer*)((char*)block + sizeof(Block) + block->size);
}


void validate_heap() 
{
    Block *current = head;
    size_t count = 0;
    
    while(current) 
    {
        
        if(count++ > 1000) 
        {
            fprintf(stderr, "Possible infinite loop in block list\n");
            assert(0);
        }
        
        
        if(!current->is_mmap && ((void*)current < (void*)head || (void*)current > sbrk(0)))
        {
            fprintf(stderr, "Block %p outside heap boundaries\n", (void*)current);
            assert(0);
        }
        
        if(current->is_mmap)
        {
            
            if(current->magic != ALLOC_MAGIC && current->magic != FREED_MAGIC)
            {
                fprintf(stderr, "Invalid mmap block at %p\n", (void*)current);
                assert(0);
            }
        }
        else 
        {
           
            if(current->magic != ALLOC_MAGIC && current->magic != FREED_MAGIC) 
            {
                fprintf(stderr, "Invalid magic in block %p: 0x%lx\n", 
                      (void*)current, current->magic);
                assert(0);
            }
        }
        
        
        if(current->next)
        {
            
            if (current->next && !current->next->is_mmap && ((void*)current->next < (void*)head || (void*)current->next > sbrk(0)))
            {
                fprintf(stderr, "Invalid next pointer in block %p\n", (void*)current);
                assert(0);
            }
            
           
            if(current->next->prev != current) 
            {
                fprintf(stderr, "Invalid next->prev link in block %p\n", (void*)current);
                assert(0);
            }
        }
        
        
        Block *fast = current->next ? current->next->next : NULL;
        if(fast == current) 
        {
            fprintf(stderr, "Circular reference detected at %p\n", (void*)current);
            assert(0);
        }
        
        current = current->next;
    }
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
        memset(block,0,sizeof(Block));
        block->magic = ALLOC_MAGIC;
        block->size = size;
        block->free = false;
        block->next = NULL;
        block->prev = tail;
        block->is_mmap = true;
    }
    else
    {
        size_t page_size = getpagesize();
        size_t full_block = sizeof(Block) + size + sizeof(Footer);
        size_t request_size = ((full_block + page_size - 1) / page_size) * page_size;
        
        request = sbrk(request_size);
        if (request == (void*)-1) return NULL;
        
        block = (Block*)request;
        memset(block, 0, sizeof(Block));
        block->magic = ALLOC_MAGIC;
        block->size = request_size - sizeof(Block) - sizeof(Footer);
        block->free = false;
        block->next = NULL;
        block->prev = tail;
        block->is_mmap = false;
    }

    Footer *foot = get_Footer(block);
    if(!foot)
    {
        if(block->is_mmap)
        {
            munmap(block, block->size + sizeof(Block) + sizeof(Footer));
        }
        return NULL;
    }
    foot->size = block->size;
    
    // Update global list
   if (!head) head = block;
    
   if (tail)
    {
        tail->next = block;
        block->prev = tail;  // Make sure to set prev pointer
    } 
    else block->prev = NULL;  // First block has no previous

    tail = block;
    
    return block;
}

void split(Block *block, size_t size)
{
    size_t remaining_size = block->size - size;

    if(block->is_mmap || (remaining_size < MIN_BLOCK_SIZE)) return;

    Block *new_block = (Block*)((char*)block + sizeof(Block) + size);

    new_block->magic = ALLOC_MAGIC;
    new_block->size = block->size - size - sizeof(Block) - sizeof(Footer);
    new_block->free = true;
    new_block->next = block->next;
    new_block->prev = block;
    new_block->is_mmap = false;

    block->size = size;
    block->next = new_block;

    Footer *new_footer = get_Footer(new_block);
    if(!new_footer)
    {
        fprintf(stderr, "Failed to get footer for new block at %p\n", (void*)new_block);
        return;
    }
    new_footer->size = new_block->size;

    Footer *block_footer = get_Footer(block);
    if(!block_footer)
    {
        fprintf(stderr, "Failed to get footer for block at %p\n", (void*)block);
        return;
    }
    block_footer->size = block->size;

    if (new_block->next) new_block->next->prev = new_block;
    else tail = new_block;
}



void* my_malloc(size_t size)
{
    pthread_mutex_lock(&alloc_mutex);
    validate_heap(); // Validate the heap before allocation
    Block *block;
    if(size <= 0 || size > SIZE_MAX - sizeof(Block) - sizeof(Footer)) 
    {
        pthread_mutex_unlock(&alloc_mutex);
        fprintf(stderr,"Overflow or underflow in my_malloc with size %zu\n", size);
        return NULL; //Invalid size
    }
    size_t  actual_size = ALIGN(size);
    //size_t  total_size = sizeof(Block) + actual_size + sizeof(Footer);

    block = find_best_fit(actual_size);
    if(!block)
    {
        block = request_space(actual_size);
        if(!block)  
        {
            pthread_mutex_unlock(&alloc_mutex);
            return NULL;
        }
    }
    else
    {
        if(block->size >= actual_size + MIN_BLOCK_SIZE) split(block, actual_size);
        block->free = false; //Mark the block as used
    }
    validate_heap();
    pthread_mutex_unlock(&alloc_mutex);
    return (void*)((char*)block + sizeof(Block)); //Return a pointer to the memory after the block header

}

//TODO: REALLOC
void *my_calloc(size_t nmemb, size_t size)
{
    if (nmemb == 0 || size == 0) return NULL;

    if (size > SIZE_MAX / nmemb) return NULL;

    size_t total_size = nmemb * size;

    if (total_size > SIZE_MAX - sizeof(Block) - sizeof(Footer)) return NULL;

    void *ptr = my_malloc(total_size);
    if (!ptr) return NULL;

    memset(ptr, 0, total_size);
    return ptr;
}





void coalesce_blocks(Block *block)
{
    validate_heap();
    if(!block || !block->free) return;

    if (!block || block->magic != FREED_MAGIC) return;

    
    if (block->prev && block->prev->free && 
        (char*)block->prev + sizeof(Block) + block->prev->size == (char*)block) {
        
        block->prev->size += sizeof(Block) + block->size;
        
        Footer *foot = get_Footer(block->prev);
        if (foot) foot->size = block->prev->size;
        
        block->prev->next = block->next;
        if (block->next) block->next->prev = block->prev;
        block = block->prev;
    }

    
    if (block->next && block->next->free &&
        (char*)block + sizeof(Block) + block->size == (char*)block->next) {
        
        block->size += sizeof(Block) + block->next->size;
        
        Footer *foot = get_Footer(block);
        if (foot) foot->size = block->size;
        
        block->next = block->next->next;
        if (block->next) block->next->prev = block;
    }

    validate_heap();
}


Block *get_block_ptr(void *ptr) 
{
  if(!ptr) return NULL;
    Block* block = (Block*)((char*)ptr - sizeof(Block));
    if(block->magic != ALLOC_MAGIC && block->magic != FREED_MAGIC) return NULL;
    return block;
}

void my_free(void* ptr)
{
    pthread_mutex_lock(&alloc_mutex);
    validate_heap(); // Validate the heap before freeing
    if(!ptr) 
    {
        pthread_mutex_unlock(&alloc_mutex);
        return; //Invalid posize_ter
    }

    Block *block_ptr = get_block_ptr(ptr);
    if(!block_ptr || (block_ptr->magic != ALLOC_MAGIC && block_ptr->magic != FREED_MAGIC)) 
    {
        pthread_mutex_unlock(&alloc_mutex);
        return;
    }

    if(block_ptr->free) 
    {
        pthread_mutex_unlock(&alloc_mutex);
        return;
    }
    

    if(block_ptr->is_mmap) 
    {
        block_ptr->magic = FREED_MAGIC;

        // Remove the block from the linked list
        if (block_ptr->prev) block_ptr->prev->next = block_ptr->next;
        else head = block_ptr->next;

        if (block_ptr->next) block_ptr->next->prev = block_ptr->prev;
        else tail = block_ptr->prev;

        //Unmap the memory
        munmap(block_ptr, block_ptr->size + sizeof(Block) + sizeof(Footer));
    
        pthread_mutex_unlock(&alloc_mutex);
        return;
    }
   block_ptr->magic = FREED_MAGIC;
    block_ptr->free = true;
    coalesce_blocks(block_ptr);

    validate_heap();
    pthread_mutex_unlock(&alloc_mutex);
}

// Function to print memory statistics
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