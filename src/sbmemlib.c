
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <semaphore.h>
 
// DEFINITIONS
#define MBMEM_NAME "/sbmemlib"
#define MIN_SEG_SIZE 32000
#define MAX_SEG_SIZE 256000
#define MIN_REQ_SIZE 128
#define MAX_REQ_SIZE 4096
#define errExit(msg)        \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0);

// FUNCTIONS' PROTOTYPES
struct Head *find_buddy(struct Head *block);
struct Head *merge_buddies(struct Head *buddy1, struct Head *buddy2);
void split_chunck(struct Head *left_chunck);
void print_memory();
int is_pow2(int val);
struct Head *get_next(struct Head * cur);

// STRUCTURES
struct Head {
    int is_alloc;
    int size;
};

struct SharedMemInfo {
    int size;
    sem_t semaphore;
};

// GLOBAL VARIABLES
int *pointerToSharedSegment = NULL;
struct SharedMemInfo *info;

// IMPLEMENTATION 

/**
 * Used to initialize the library (called only once)
 * @param segmentSize The desired size of the shared memory
 * */
int sbmem_init(int segmentsize) {
    if (!is_pow2(segmentsize))
        errExit("[-] Segment size must be a power of 2.\n");

    if (segmentsize > MAX_SEG_SIZE || segmentsize < MIN_SEG_SIZE)
        errExit("[-] Segment size must be between 32KB and 256KB.\n");

    int shm_fd = shm_open(MBMEM_NAME, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (shm_fd == -1)
        errExit("An error occured while creating shared memory");

    if (ftruncate(shm_fd, segmentsize + sizeof(struct SharedMemInfo)) != 0)
        errExit("An error occured while creating shared memory");

    void *info_and_head = mmap(0, sizeof(struct Head) + sizeof(struct SharedMemInfo), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (info_and_head == MAP_FAILED)
        errExit("An error occured mmapping shared memory");

    ((struct SharedMemInfo*)info_and_head)->size = segmentsize;

    int ret = sem_init(&(((struct SharedMemInfo*)info_and_head)->semaphore), 1, 1);
    if (ret != 0)
        errExit("Failed creating a semaphore");

    info_and_head = (char *)info_and_head + sizeof(struct SharedMemInfo);
    
    ((struct Head *)info_and_head)->is_alloc = 0;
    ((struct Head *)info_and_head)->size = segmentsize;

    return (0);
}

/**
 * To dealocate the shared memory (called only once)
 * */
int sbmem_remove() {
    sem_destroy(&info->semaphore);
    if (shm_unlink(MBMEM_NAME) == -1) { 
        errExit("An error occured while unlinking the shared memory!");
    }
    
    return (0);
}

/**
 * To map the shared memory to the processes's virtual adrees
 * (called once per process)
 * */
int sbmem_open() {
    // Open shared memory
    int shm_fd = shm_open(MBMEM_NAME, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    
    if (shm_fd == -1) 
        errExit("An error occured while creating shared memory");

    // Map and read the size information of shared memory
    info = mmap(0, sizeof(struct SharedMemInfo), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
   
    if (info == MAP_FAILED)
        errExit("An error occured mmapping shared memory");

    // Map the whole shared memory
    pointerToSharedSegment = mmap(0, info->size + sizeof(struct SharedMemInfo), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (pointerToSharedSegment == MAP_FAILED)
        errExit("An error occured mmapping shared memory");

    // Move the pointer so that it points to the begining of the block    
    pointerToSharedSegment = (int *)((char *)pointerToSharedSegment + sizeof(struct SharedMemInfo));

    return (0);
}

/**
 * Allocates the desired amount of memory using 
 * buddy allocation algorithm
 * @param size The size of the memory wanted
 * @return A pointer to the address of the 
 *         memory allocated or NULL if allocation 
 *         is not possible
 * */
void *sbmem_alloc(int size) {
    sem_wait(&(info->semaphore));

    size += sizeof(struct Head);
    
    struct Head *tmpPointer = ((struct Head *) pointerToSharedSegment);
    void *ptr = NULL;
    
    do
    {   // Find a suitable block and split blocks accordingly
        if ((tmpPointer->size/2) >= size && tmpPointer->is_alloc == 0){
            split_chunck(tmpPointer);
        } else if (tmpPointer->is_alloc == 1 || tmpPointer->size < size) {
            tmpPointer = get_next(tmpPointer);
        } else {
            tmpPointer->is_alloc = 1;
            ptr = tmpPointer + 1;
            break;
        }
    } while (tmpPointer != NULL);

    // print_memory();

    sem_post(&(info->semaphore));
    return (ptr);
}

/**
 * Frees the memory allocated
 * @param p A pointer to the begining of the allocated memory
 */
void sbmem_free(void *p) {
    if (p == NULL)
        return;

    sem_wait(&(info->semaphore));

    // printf("Frying P = %d of size %d \n", p, ((struct Head *)p)[-1].size);
    struct Head *block = ((struct Head *)p)-1;
    block->is_alloc = 0;

    // Find buddy
    struct Head *buddy = find_buddy(block);
    while (buddy->is_alloc != 1 && buddy->size == block->size){
        // Merge buddy
        block = merge_buddies(buddy, block);
        buddy = find_buddy(block);
    }

    // print_memory();
    sem_post(&(info->semaphore));
}

int sbmem_close() {
    munmap(pointerToSharedSegment - sizeof(struct SharedMemInfo), info->size + sizeof(int));
    return (0);
}

/**
 * Finds the buddy of a given block
 * A buddy: is a block of the same size of the given block, 
 * where the two blocks were part of one larger block of 
 * size 2^k+1 if the smaller blocks are of size 2^k
 * @param block The block we want to find its buddy
 */
struct Head *find_buddy(struct Head *block) {
    int size_class = log2(block->size);

    // if the address of the given block (the block of size 2^K) % 2^k+1 then its buddy is at: (current block's address) + 2^k
    // else it is at (current block's address) - 2^k
    if (((char *)block - (char *)pointerToSharedSegment) % (long)pow(2, size_class + 1) == 0){
        return (struct Head *)((char *)block + block->size);
    } else {
        return (struct Head *)((char *)block - block->size);
    }
}

/**
 * Merges back two blocks that were already 
 * one larger block (buddies).
 * @param buddy1 The first block
 * @param buddy2 The second block
 * @return The new merged block
 */
struct Head *merge_buddies(struct Head *buddy1, struct Head *buddy2) {
    int size_class = log2(buddy1->size);
    struct Head *buddy_left;
    struct Head *buddy_right;

    if (((char *)buddy1 - (char *)pointerToSharedSegment) % (long)pow(2, size_class + 1) == 0){
        buddy_left = buddy1;
        buddy_right = buddy2;
    } else {
        buddy_left = buddy2;
        buddy_right = buddy1;
    }

    buddy_left->size += buddy_right->size;

    return buddy_left;
}

/**
 * Splits a memory block in half
 * @param block The memory block to be splitted
 * */
void split_chunck(struct Head *block) {
    struct Head *buddy = block + (block->size / 2)/sizeof(struct Head);
    buddy->size = block->size / 2;
    buddy->is_alloc = 0;

    block->size = block->size / 2;
}

/**
 * Prints the memory contents
 * */
void print_memory() {
    struct Head *mem_pointer = (struct Head *) pointerToSharedSegment;
    
    printf("---------------------------\n");
    printf("     MEMORY CONTENT        \n");
    printf("___________________________\n");
    while (mem_pointer != NULL)
    {
        printf("| Addr:           \b %p \b\n", mem_pointer);
        printf("| size:           \b %d \b\n", mem_pointer->size);
        printf("| is Allocated:   \b %d \b\n", mem_pointer->is_alloc);
        printf("| next's address2: \b %p \b\n", get_next(mem_pointer));
        printf("___________________________\n");    

        mem_pointer = get_next(mem_pointer);
    }
    printf("___________________________\n");    
}

/**
 *  Checks if the given number is a power of 2 
 * */
int is_pow2(int val) {
    return (val != 0) && ((val & (val - 1)) == 0);
}

/**
 * Calculated the address of the next block
 * @return Pointer to the address of the next block or NULL if the isn't any
 * */
struct Head *get_next(struct Head * cur) {
    struct Head *next = (struct Head *)((char *)cur + cur->size);
    if (next < (struct Head *)(((char *)pointerToSharedSegment) + info->size))
        return next;
    else
        return NULL;
}