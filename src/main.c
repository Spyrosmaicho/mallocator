#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "my_allocator.h"
#include <stdint.h>

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


                /*MALLOC TESTS*/
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

void test_random_allocations() 
{
    print_test_header("Random Allocation Stress Test");

    #define NUM_ALLOCS 100
    #define MAX_SIZE (4 * 1024 * 1024) // 4 MB max size for allocations
    srand(time(NULL)); 
    void *pointers[NUM_ALLOCS] = {0};
    size_t sizes[NUM_ALLOCS] = {0};

    for (int i = 0; i < NUM_ALLOCS; i++) 
    {
        sizes[i] = (size_t)(rand() % MAX_SIZE + 1);
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

    for (int i = 0; i < NUM_ALLOCS / 2; i++) 
    {
        int idx;
        do {
            idx = rand() % NUM_ALLOCS;
        } while (pointers[idx] == NULL);
        my_free(pointers[idx]);
        pointers[idx] = NULL;
    }
    printf("Freed half of blocks randomly: ");
    print_test_result(1);

    for (int i = 0, allocated = 0; allocated < NUM_ALLOCS / 4 && i < NUM_ALLOCS; i++) 
    {
        if (pointers[i] == NULL) 
        {
            sizes[i] = (size_t)(rand() % MAX_SIZE + 1);
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

    putchar('\n');
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
    putchar('\n');
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
            /*CALLOC TESTS*/
void test_calloc_overflow() {
    print_test_header("calloc Overflow Test");
    
    void *p = my_calloc(SIZE_MAX, 4);
    printf("Overflow allocation (SIZE_MAX*4): ");
    print_test_result(p == NULL);
    if(p) my_free(p);

    size_t safe_nmemb = 512;
    size_t safe_size = 1000; // Safe values to avoid overflow
    void *p2 = my_calloc(safe_nmemb, safe_size);
    printf("Safe allocation (%zu x %zu): ", safe_nmemb, safe_size);
    print_test_result(p2 != NULL);
    if(p2) my_free(p2);
}

void test_calloc_zero_initialization() {
    print_test_header("calloc Zero-Initialization");
    
    #define TEST_SIZE 1024
    int *arr = (int*)my_calloc(TEST_SIZE, sizeof(int));
    if(!arr) 
    {
        print_test_result(0);
        return;
    }
    
    int is_zeroed = 1;
    for(size_t i = 0; i < TEST_SIZE; i++) 
    {
        if(arr[i] != 0) 
        {
            is_zeroed = 0;
            break;
        }
    }
    
    printf("Memory zeroed: ");
    print_test_result(is_zeroed);
    my_free(arr);
}

void test_calloc_zero_parameters() 
{
    print_test_header("calloc Zero Parameters");
    
    void *p1 = my_calloc(0, 100);
    void *p2 = my_calloc(100, 0);
    void *p3 = my_calloc(0, 0);
    
    printf("Zero nmemb: ");
    print_test_result(p1 == NULL);
    printf("Zero size: ");
    print_test_result(p2 == NULL);
    printf("Both zero: ");
    print_test_result(p3 == NULL);
}

void test_calloc_coalescing() 
{
    print_test_header("calloc Coalescing Test");
    
    void *p1 = my_calloc(10, sizeof(int));
    void *p2 = my_calloc(10, sizeof(int));
    if(!p1 || !p2) 
    {
        print_test_result(0);
        return;
    }
    
    my_free(p1);
    my_free(p2);
    
    void *p_large = my_calloc(20, sizeof(int));
    printf("Coalesced allocation: ");
    print_test_result(p_large != NULL);
    
    if(p_large) 
    {
        printf("Verify contents: ");
        int *arr = (int*)p_large;
        int valid = 1;
        for(int i = 0; i < 20; i++) 
        {
            if(arr[i] != 0) 
            {
                valid = 0;
                break;
            }
        }
        print_test_result(valid);
        my_free(p_large);
    }
}

void test_calloc_random() 
{
    print_test_header("calloc Random Stress Test - Extreme Edition");
    
    srand(time(NULL));
    int passed = 1;
    int total_tests = 75;  // Increased from 50 to 100 for more stress
    size_t max_failures = 5;  // Allow up to 5 failures before aborting
    size_t failures = 0;
    
    printf("Running %d random allocations...\n", total_tests);
    
    for(int i = 0; i < total_tests; i++) 
    {
        // More extreme random sizes
        size_t nmemb = (rand() % 1000 + 1) * (rand() % 10 + 1);
        size_t size = (rand() % 1024 + 1) * (rand() % 8 + 1);
        
        int *p = (int*)my_calloc(nmemb, size);
        if(!p) 
        {
            printf("FAILED (out of memory?)\n");
            failures++;
            passed = 0;
            if(failures >= max_failures) 
            {
                printf("Too many failures (%zu), aborting test...\n", failures);
                break;
            }
            continue;
        }
        
        // Verify zero-initialization
        int zero_check_passed = 1;
        for(size_t j = 0; j < (nmemb * size)/sizeof(int); j++) 
        {
            if(p[j] != 0) 
            {
                zero_check_passed = 0;
                break;
            }
        }
        
        if(!zero_check_passed) 
        {
            printf("FAILED (memory not zeroed)\n");
            passed = 0;
        }
        
        my_free(p);
    }
    
    print_test_result(passed);
    if(failures > 0) 
    {
        printf("Note: %zu allocations failed (possibly due to memory constraints)\n", failures);
    }
}

            /*REALLOC TESTS*/
void test_realloc_basic() {
    print_test_header("Basic Realloc Test");
    
    // Allocate initial block
    int *ptr = (int*)my_malloc(10 * sizeof(int));
    if (!ptr) {
        print_test_result(0);
        return;
    }
    
    // Initialize data
    for (int i = 0; i < 10; i++) ptr[i] = i;
    
    // Realloc to larger size
    int *new_ptr = (int*)my_realloc(ptr, 20 * sizeof(int));
    int result = (new_ptr != NULL);
    printf("Realloc to larger size: ");
    print_test_result(result);
    
    if (result) {
        // Verify old data
        int data_ok = 1;
        for (int i = 0; i < 10; i++) {
            if (new_ptr[i] != i) {
                data_ok = 0;
                break;
            }
        }
        printf("Data preservation check: ");
        print_test_result(data_ok);
        
        // Test writing new data
        for (int i = 10; i < 20; i++) new_ptr[i] = i;
        printf("New data write check: ");
        print_test_result(1);
        
        my_free(new_ptr);
    }
}

void test_realloc_smaller() {
    print_test_header("Realloc Smaller Size Test");
    
    int *ptr = (int*)my_malloc(20 * sizeof(int));
    if (!ptr) {
        print_test_result(0);
        return;
    }
    
    for (int i = 0; i < 20; i++) ptr[i] = i;
    
    // Realloc to smaller size
    int *new_ptr = (int*)my_realloc(ptr, 10 * sizeof(int));
    int result = (new_ptr != NULL);
    printf("Realloc to smaller size: ");
    print_test_result(result);
    
    if (result) {
        // Verify data was preserved
        int data_ok = 1;
        for (int i = 0; i < 10; i++) {
            if (new_ptr[i] != i) {
                data_ok = 0;
                break;
            }
        }
        printf("Data preservation check: ");
        print_test_result(data_ok);
        
        my_free(new_ptr);
    }
}

void test_realloc_null_ptr() {
    print_test_header("Realloc with NULL Pointer");
    
    // Should behave like malloc
    int *ptr = (int*)my_realloc(NULL, 100);
    printf("Realloc NULL with size (should malloc): ");
    print_test_result(ptr != NULL);
    
    if (ptr) my_free(ptr);
}

void test_realloc_zero_size() {
    print_test_header("Realloc with Zero Size");
    
    int *ptr = (int*)my_malloc(100);
    if (!ptr) {
        print_test_result(0);
        return;
    }
    
    // Should behave like free
    int *new_ptr = (int*)my_realloc(ptr, 0);
    printf("Realloc with size=0 (should free): ");
    print_test_result(new_ptr == NULL);
}

void test_realloc_coalescing() {
    print_test_header("Realloc Coalescing Test");
    
    void *p1 = my_malloc(100);
    void *p2 = my_malloc(100);
    void *p3 = my_malloc(100);
    
    if (!p1 || !p2 || !p3) {
        print_test_result(0);
        return;
    }
    
    printf("Initial allocations:\n");
    print_memory_stats();
    
    // Free middle block
    my_free(p2);
    printf("\nAfter freeing middle block:\n");
    print_memory_stats();
    
    // Realloc first block - should coalesce with freed p2
    void *new_p1 = my_realloc(p1, 250);
    printf("\nAfter realloc p1 to 250 bytes (should coalesce):\n");
    print_memory_stats();
    
    int result = (new_p1 != NULL);
    printf("Realloc result: ");
    print_test_result(result);
    
    if (new_p1) {
        my_free(new_p1);
        my_free(p3);
    }
}

void test_realloc_edge_cases() {
    print_test_header("Realloc Edge Cases");
    
    printf("Realloc that first 50 bytes should be preserved: ");
    void *ptr1 = my_malloc(100);
    memset(ptr1, 0xAB, 100);
    void *res = my_realloc(ptr1, 50);
    //First 50 bytes should be preserved
    print_test_result(memcmp(ptr1, res, 50) == 0);

    // Pointers should be the same
    /*void *ptr2 = my_malloc(80);
    void *res2 = my_realloc(ptr2, 80);
    print_test_result(res2 == ptr2);*/
    
    // Realloc with very large size
    printf("Realloc with SIZE_MAX (should fail): ");
    void *huge_ptr = my_realloc(my_malloc(100), SIZE_MAX);
    print_test_result(huge_ptr == NULL);
}

void test_realloc_random() {
    print_test_header("Random Realloc Stress Test");
    
    #define NUMS_ALLOCS 50
    void *ptrs[NUMS_ALLOCS] = {0};
    size_t sizes[NUMS_ALLOCS] = {100, 200, 300, 400, 500}; // Repeat pattern
    
    // Initial allocations
    for (int i = 0; i < NUMS_ALLOCS; i++) {
        ptrs[i] = my_malloc(sizes[i % 5]);
        if (!ptrs[i]) {
            printf("Initial alloc failed at %d\n", i);
            print_test_result(0);
            return;
        }
    }
    
    // Random reallocs
    srand(time(NULL));
    int passed = 1;
    for (int i = 0; i < NUMS_ALLOCS * 2; i++) {
        int idx = rand() % NUMS_ALLOCS;
        size_t new_size = (rand() % 1000) + 1;
        
        void *old_ptr = ptrs[idx];
        ptrs[idx] = my_realloc(ptrs[idx], new_size);
        
        if (!ptrs[idx]) {
            printf("Realloc failed at iter %d (idx %d, size %zu)\n", 
                  i, idx, new_size);
            passed = 0;
            ptrs[idx] = old_ptr; // Restore if realloc failed
        }
    }
    
    printf("Random realloc operations: ");
    print_test_result(passed);
    
    // Cleanup
    for (int i = 0; i < NUMS_ALLOCS; i++) {
        if (ptrs[i]) my_free(ptrs[i]);
    }
}


int main() 
{
    printf("%sStarting Memory Allocator Test Suite%s\n\n", COLOR_GREEN, COLOR_RESET);
    
    //Malloc tests
    test_basic_allocation();
    test_large_mmap_allocation();
    test_array_allocation();
    test_random_allocations();
    test_edge_cases();
    test_coalescing();
    
    //Calloc tests
    test_calloc_overflow();
    test_calloc_zero_initialization();
    test_calloc_zero_parameters();
    test_calloc_coalescing();
    test_calloc_random();

    //Realloc tests
    test_realloc_basic();
    test_realloc_smaller();
    test_realloc_null_ptr();
    test_realloc_zero_size();
    test_realloc_coalescing();
    test_realloc_edge_cases();
    test_realloc_random();

    printf("\n%sAll tests completed!%s\n", COLOR_GREEN, COLOR_RESET);
    
    return 0;
}