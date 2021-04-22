

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "sbmem.h"

int main()
{
    int i, ret;
    char *p;
    char *q;
    char *r;
    char *s;
    char *t;
    char *u;

    printf("Calling sbmem_open\n");
    ret = sbmem_open();
    if (ret == -1)
        exit(1);

    p = sbmem_alloc(8192-16); // allocate space to hold 1024 characters
    q = sbmem_alloc(4096-16); // allocate space to hold 1024 characters
    sbmem_free(q);
    p = sbmem_alloc(8192-16); // allocate space to hold 1024 characters
    
    r = sbmem_alloc(512-16); // allocate space to hold 1024 characters

    s = sbmem_alloc(256-16); // allocate space to hold 1024 characters

    t = sbmem_alloc(256-16); // allocate space to hold 1024 characters

    sbmem_free(s);
    sbmem_free(t);

    t = sbmem_alloc(512-16); // allocate space to hold 1024 characters
    if (t == NULL){
        printf("Could not allocate! \n");
        exit(1);
    }
    t = sbmem_alloc(256-16); // allocate space to hold 1024 characters

    sbmem_close();


    return (0);
}
