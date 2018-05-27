#include "disk_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fail(char *m)
{
    printf(" >> TEST FAILED - %s\n", m);
    exit(1);
}

char data[BLOCK_SIZE];
char out[BLOCK_SIZE];

int main(int argc, char** argv)
{
    int r;
    DiskDriver dd;
    DiskDriver_init(&dd, "driver.disk", 10);
    
    r = DiskDriver_getFreeBlock(&dd, dd.header->first_free_block);
    
    strcpy(data, "ciao mondo");
    DiskDriver_writeBlock(&dd, data, r);
    
    DiskDriver_readBlock(&dd, out, r);
    if(strcmp(out, "ciao mondo"))
        fail("DiskDriver_readBlock(&dd, out, r)");
    
    printf(" >> TEST PASSED!\n");
    return 0;
}
