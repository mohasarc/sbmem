
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h> 

// Define a name for your shared memory; you can give any name that start with a slash character; it will be like a filename.
#define MBMEM_NAME "/sbmemlib"

// Define semaphore(s)

// Define your stuctures and variables. 

int sbmem_init(int segmentsize)
{
    int res = shm_open(MBMEM_NAME, O_RDWR | O_CREAT | O_TRUNC,  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    
    if (res == -1) {
        printf("An error occured while creating shared memory");
        return(1);
    } else {
        printf ("sbmem init called"); // remove all printfs when you are submitting to us.  
        return (0); 
    }
}

int sbmem_remove()
{

    return (0); 
}

int sbmem_open()
{

    return (0); 
}


void *sbmem_alloc (int size)
{
    return (NULL);
}


void sbmem_free (void *p)
{

 
}

int sbmem_close()
{
    
    return (0); 
}
