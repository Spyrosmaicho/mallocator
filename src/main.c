#include <stdio.h>
#include "my_malloc.h"
#include <stdlib.h>

int main(void)
{
    void *ptr1 = my_malloc(100);
    if(!ptr1)
    {
        fprintf(stderr,"Memory allocation failed for ptr1\n");
        return 1;
    }

    void *ptr2 = my_malloc(200);
    if(!ptr2)
    {
        my_free(ptr1);
        fprintf(stderr,"Memory allocation failed for ptr2\n");
        return 1;   
    }

    void *ptr3 = my_malloc(4097); // This should trigger mmap allocation
    if(!ptr3)
    {
        my_free(ptr1);
        my_free(ptr2);
        fprintf(stderr,"Memory allocation failed for ptr3\n");
    }

    // Print memory statistics
    print_memory_stats();
    return 0;
}