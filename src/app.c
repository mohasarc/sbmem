

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

    // while (1)
    // {   
        printf("allocating p 27\n");
        p = sbmem_alloc(8192-16); // allocate space to hold 1024 characters
        printf("allocating q 29\n");
        q = sbmem_alloc(4096-16); // allocate space to hold 1024 characters

        printf("freeing q 32\n");
        sbmem_free(q);
        printf("freeing p 34\n");
        sbmem_free(p);

        printf("allocating p 34\n");
        p = sbmem_alloc(8192-16); // allocate space to hold 1024 characters
        
        printf("allocating r 37\n");
        r = sbmem_alloc(512-16); // allocate space to hold 1024 characters

        printf("freeing r 43\n");
        sbmem_free(r);

        printf("allocating s 40\n");
        s = sbmem_alloc(256-16); // allocate space to hold 1024 characters

        printf("allocating t 43\n");
        t = sbmem_alloc(256-16); // allocate space to hold 1024 characters

        printf("freeing p 49\n");
        sbmem_free(p);
        printf("freeing t 55\n");
        sbmem_free(t);
        printf("freeing s 53\n");
        sbmem_free(s);

        printf("allocating t 58\n");
        t = sbmem_alloc(512-16); // allocate space to hold 1024 characters

        printf("allocating u 53\n");
        u = sbmem_alloc(256-16); // allocate space to hold 1024 characters

        printf("freeing t 64\n");
        sbmem_free(t);
        printf("freeing u 66\n");
        sbmem_free(u);
    // }

    sbmem_close();
    return (0);
}
