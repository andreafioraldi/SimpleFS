#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(int argc, char** argv)
{
    SimpleFS sfs;
    DiskDriver disk;

    DiskDriver_init(&disk, "test.disk", 256);
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
    for(i = 0; i < 5; ++i) {
        sprintf(name, "file%d", i);
        SimpleFS_createFile(root, name);
    }
    
    printf(" >> SimpleFS_readDir\n");
    char **names = malloc(5*sizeof(char*));
    SimpleFS_readDir(names, root);
    for(i = 0; i < 5; ++i) {
    	printf("%d   %s\n", i, names[i]);
        sprintf(name, "file%d", i);
        assert(!strcmp(names[i], name));
    }
    
    printf(" >> SimpleFS_openFile\n");
    FileHandle *fh = SimpleFS_openFile(root, "not-exists");
    assert(fh==NULL);
    fh = SimpleFS_openFile(root, "file2");
    assert(fh!=NULL);
    
    printf(" >> SimpleFS_write\n");
    int w = SimpleFS_write(fh, "pippo", sizeof("pippo"));
    assert(w == sizeof("pippo"));
    
    char *junk="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "bbbbbbbbbbbbbbbbbbbb";
    w = SimpleFS_write(fh, junk, 600);
    assert(w == 600);
    
    fh->pos_in_file -= 20; //use seek when done
    
    printf(" >> SimpleFS_read\n");
    char tmp[21];
    int v = SimpleFS_read(fh, (void*) tmp, 20);
    tmp[20] = 0;
    
    printf("readed: %d\n", v);
    printf("%s\n", tmp);
    
    
}
