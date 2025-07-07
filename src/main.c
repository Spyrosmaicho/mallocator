#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "my_malloc.h"

#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_RESET "\033[0m"

void print_test_header(const char *description) 
{
    printf("\n%s----- %s -----%s\n", COLOR_GREEN, description, COLOR_RESET);
}

void print_test_result(int passed) 
{
    (passed) ? printf("%s[PASSED]%s\n", COLOR_GREEN, COLOR_RESET) : printf("%s[FAILED]%s\n", COLOR_RED, COLOR_RESET);
}

void test_basic_allocation() 
{
    print_test_header("Basic Allocation Test");
    
    void *ptr = my_malloc(100);
    int result = (ptr != NULL);
    printf("Allocation of 100 bytes: ");
    print_test_result(result);
    
    if (result) 
    {
        my_free(ptr);
        printf("Free operation: ");
        print_test_result(1);
    }
}

void test_large_mmap_allocation() 
{
    print_test_header("Large MMAP Allocation Test");
    
    void *ptr = my_malloc(4097); // Should trigger mmap
    int result = (ptr != NULL);
    printf("MMAP allocation (4097 bytes): ");
    print_test_result(result);
    
    if (result) 
    {
        my_free(ptr);
        printf("MMAP free operation: ");
        print_test_result(1);
    }
}

void test_array_allocation() 
{
    print_test_header("Array Allocation Test");
    
    int *array = (int *)my_malloc(100 * sizeof(int));
    int result = (array != NULL);
    printf("Array allocation (100 ints): ");
    print_test_result(result);
    
    if (result) 
    {
        //Test writing to array
        for (int i = 0; i < 100; i++) array[i] = i;
        
        //Test reading from array
        int read_ok = 1;
        for (int i = 0; i < 100; i++) 
        {
            if (array[i] != i) 
            {
                read_ok = 0;
                break;
            }
        }
        printf("Array read/write test: ");
        print_test_result(read_ok);
        
        my_free(array);
    }
}

//ΤΟDO: MAKE IT RANDOM BECAUSE IT IS NOT RANDOM
void test_random_allocations() 
{
    print_test_header("Random Allocation Stress Test");

    #define NUM_ALLOCS 100

    void *pointers[NUM_ALLOCS] = {0};
    size_t sizes[NUM_ALLOCS] = {0};

    // Allocate random blocks and fill them
    for (int i = 0; i < NUM_ALLOCS; i++) 
    {
        sizes[i] = 2* i + 1; 
        pointers[i] = my_malloc(sizes[i]);
        if (!pointers[i]) 
        {
            printf("Failed allocation at iteration %d\n", i);
            print_test_result(0);
            return;
        }
        memset(pointers[i], 0xFF, sizes[i]);
    }
    printf("Allocated %d random blocks: ", NUM_ALLOCS);
    print_test_result(1);

    //Free randomly about half of the blocks
    for (int i = 0; i < NUM_ALLOCS / 2; i++) 
    {
        int idx;
        do {
            idx = rand() % NUM_ALLOCS;
        } while (pointers[idx] == NULL); // Ensure we only free allocated blocks
        my_free(pointers[idx]);
        pointers[idx] = NULL;
    }
    printf("Freed half of blocks randomly: ");
    print_test_result(1);

    //Re-allocate in freed positions about 1/4 of total
    for (int i = 0, allocated = 0; allocated < NUM_ALLOCS / 4 && i < NUM_ALLOCS; i++) 
    {
        if (pointers[i] == NULL) 
        {
            sizes[i] = 2* i +1;
            pointers[i] = my_malloc(sizes[i]);
            if (!pointers[i]) 
            {
                printf("Failed re-allocation at iteration %d\n", i);
                print_test_result(0);
                return;
            }
            allocated++;
        }
    }
    printf("Re-allocated some blocks: ");
    print_test_result(1);

    // Free all remaining allocated blocks
    for (int i = 0; i < NUM_ALLOCS; i++) 
    {
        if (pointers[i]) 
        {
            my_free(pointers[i]);
            pointers[i] = NULL;
        }
    }
    printf("Freed all remaining blocks: ");
    print_test_result(1);

    print_memory_stats();
}


void test_edge_cases() 
{
    print_test_header("Edge Case Tests");
    
    // Test zero size
    void *ptr = my_malloc(0);
    printf("Zero-size allocation (should fail): ");
    print_test_result(ptr == NULL);
    
    // Test very large allocation
    void *huge = my_malloc((size_t)-1 / 2);
    printf("Huge allocation (should fail): ");
    print_test_result(huge == NULL);
    
    // Test free of NULL
    printf("Free NULL pointer (should handle gracefully): ");
    my_free(NULL);
    print_test_result(1);
    
    // Test double free
    void *dptr = my_malloc(100);
    if (dptr) 
    {
        my_free(dptr);
        printf("Double free detection (should handle gracefully): ");
        my_free(dptr); // This should not crash
        print_test_result(1);
    }
}

void test_coalescing() 
{
    print_test_header("Coalescing Test");
    
    void *p1 = my_malloc(100);
    void *p2 = my_malloc(100);
    void *p3 = my_malloc(100);
    
    if (!p1 || !p2 || !p3) 
    {
        print_test_result(0);
        return;
    }
    
    printf("Memory stats after allocations:\n");
    print_memory_stats();
    
    putchar('\n');

    my_free(p2);
    printf("Freed middle block:\n");
    print_memory_stats();
    
    putchar('\n');

    my_free(p1);
    printf("Freed first block (should coalesce):\n");
    print_memory_stats();
    
    putchar('\n');

    my_free(p3);
    printf("Freed last block (should coalesce):\n");
    print_memory_stats();
    
    print_test_result(1);
}


//TODO: Create more tests



int main() {
    printf("%sStarting Memory Allocator Test Suite%s\n\n", COLOR_GREEN, COLOR_RESET);
    
    test_basic_allocation();
    test_large_mmap_allocation();
    test_array_allocation();
    test_random_allocations();
    test_edge_cases();
    test_coalescing();
    
    printf("\n%sAll tests completed!%s\n", COLOR_GREEN, COLOR_RESET);
    
    return 0;
}