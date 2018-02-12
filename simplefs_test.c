#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(int argc, char** argv)
{
    SimpleFS sfs;
    DiskDriver disk;

    DiskDriver_init(&disk, "test.disk", 2048);
    DirectoryHandle *root = SimpleFS_init(&sfs, &disk);
    
    if(root == NULL || (argc > 1 && !strcmp(argv[1], "format")))
    {
        printf(" >> SimpleFS_format\n");
        SimpleFS_format(&sfs);
        root = SimpleFS_init(&sfs, &disk);
        if(argc > 1 && !strcmp(argv[1], "format"))
            return 0;
    }
    
    char name[80];
    int i;
    printf(" >> SimpleFS_createFile\n");
    for(i = 0; i < 200; ++i) {
        sprintf(name, "file%d", i);
        SimpleFS_createFile(root, name);
    }
    
    printf(" >> SimpleFS_readDir\n");
    char **names = malloc(200*sizeof(char*));
    SimpleFS_readDir(names, root);
    for(i = 0; i < 200; ++i) {
        sprintf(name, "file%d", i);
        assert(!strcmp(names[i], name));
    }
    
    printf(" >> SimpleFS_openFile\n");
    FileHandle *fh = SimpleFS_openFile(root, "not-exists");
    assert(fh==NULL);
    fh = SimpleFS_openFile(root, "file80");
    assert(fh!=NULL);
    
    
}
