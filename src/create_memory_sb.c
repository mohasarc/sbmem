

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "sbmem.h"

#define DEFAULT_SIZE 32768

int main(int argc, char *argv[])
{
    int segment_size;
    
    if (argc > 1)
        segment_size = atoi(argv[1]);
    else 
        segment_size = DEFAULT_SIZE;

    sbmem_init(segment_size); 
    return (0); 
}
