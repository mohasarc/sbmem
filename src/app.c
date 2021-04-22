

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "sbmem.h"

int main()
{
    int ret;
    char *p;
    int size_to_allocate = 250;
    printf("Calling sbmem_open\n");
    ret = sbmem_open();
    if (ret == -1)
        exit(1);

    for (int i = 0; i < 10; i++){
        printf("Requesting == %d\n", size_to_allocate);
        p = sbmem_alloc(size_to_allocate); // allocate space to hold 1024 characters
        size_to_allocate += 100;
    }
  
    sbmem_close();
    return (0);
}
