#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void fail(char *m)
{
    printf(" >> TEST FAILED - %s\n", m);
    exit(1);
}

int main(int argc, char** argv)
{
    SimpleFS sfs;
    DiskDriver disk;

    DiskDriver_init(&disk, "create.disk", 4096);
    DirectoryHandle *root = SimpleFS_init(&sfs, &disk);
    
    if(root == NULL || (argc > 1 && !strcmp(argv[1], "format")))
    {
        fprintf(stderr, " >> formatting...\n");
        SimpleFS_format(&sfs);
        root = SimpleFS_init(&sfs, &disk);
        if(argc > 1 && !strcmp(argv[1], "format"))
            return 0;
    }
    
    char name[80];
    int i;
    for(i = 0; i < 500; ++i) {
        sprintf(name, "file%d", i);
        SimpleFS_createFile(root, name);
    }
    
    for(i = 0; i < 500; ++i) {
        sprintf(name, "folder%d", i);
        SimpleFS_mkDir(root, name);
    }
    
    char **names = malloc(1000*sizeof(char*));
    SimpleFS_readDir(names, root);
    for(i = 0; i < 1000; ++i) {
    	//printf("%d   %s\n", i, names[i]);
    	if(i < 500)
            sprintf(name, "file%d", i);
        else sprintf(name, "folder%d", i - 500);
        if(strcmp(names[i], name))
            fail(name);
    }
    
    printf(" >> TEST PASSED!\n");
    return 0;
}



