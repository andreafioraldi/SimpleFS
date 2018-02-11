#include "disk_driver.h"
#include <stdio.h>

char data[BLOCK_SIZE];
char de[BLOCK_SIZE];

int main()
{
    int r;
    DiskDriver dd;
    DiskDriver_init(&dd, "test.disk", 10);
    
    r = DiskDriver_getFreeBlock(&dd, dd.header->first_free_block);
    printf("%d\n", r);
    
    strcpy(data, "ciao mondo");
    DiskDriver_writeBlock(&dd, data, r);
    
    printf("%d\n", DiskDriver_getFreeBlock(&dd, dd.header->first_free_block));
    
    DiskDriver_readBlock(&dd, de, r);
    puts(de);
    
    
}
