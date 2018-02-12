#include "simplefs.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
    SimpleFS sfs;
    DiskDriver disk;

    DiskDriver_init(&disk, "test.disk", 2048);
    
    if(argc > 1 && !strcmp(argv[1], "format"))
    {
        SimpleFS_init(&sfs, &disk);
        SimpleFS_format(&sfs);
        return 0;
    }
    
    DirectoryHandle *root = SimpleFS_init(&sfs, &disk);
    char name[80];
    int i;
    for(i = 0; i < 200; ++i) {
        sprintf(name, "file%d", i);
        SimpleFS_createFile(root, name);
    }
}
