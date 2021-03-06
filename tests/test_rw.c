#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fail(char *m)
{
    printf(" >> TEST FAILED - %s\n", m);
    exit(1);
}

int main(int argc, char** argv)
{
    SimpleFS sfs;
    DiskDriver disk;

    DiskDriver_init(&disk, "rw.disk", 2048);
    DirectoryHandle *root = SimpleFS_init(&sfs, &disk);
    
    if(root == NULL || (argc > 1 && !strcmp(argv[1], "format")))
    {
        fprintf(stderr, " >> formatting...\n");
        SimpleFS_format(&sfs);
        root = SimpleFS_init(&sfs, &disk);
        if(argc > 1 && !strcmp(argv[1], "format"))
            return 0;
    }
    
    FileHandle* empty = SimpleFS_createFile(root, "file");
    if(empty)
    {
        SimpleFS_close(empty);
    }
    
    FileHandle* fh = SimpleFS_openFile(root, "file");
    
    //45 a 8 b and others are c
    char *junk = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc";
    int r = SimpleFS_write(fh, junk, 600);
    if(r != 600) fail("SimpleFS_write(fh, junk, 600)");
    
    int pos = 45;
    int cursor = fh->pos_in_file;
    //seek back
    r = SimpleFS_seek(fh, pos);
    if(r != cursor - pos) fail("SimpleFS_seek back wrong return value");
    if(fh->pos_in_file != pos) fail("SimpleFS_seek back wrong pos");
    
    char tmp[8];
    SimpleFS_read(fh, (void*)tmp, 8);
    if(memcmp(tmp, "bbbbbbbb", 8)) fail("SimpleFS_read(fh, (void*)tmp, 8)");
    
    cursor = fh->pos_in_file;
    pos = 600;
    //seek forward
    r = SimpleFS_seek(fh, pos);
    if(r != pos - cursor) fail("SimpleFS_seek forward wrong return value");
    if(fh->pos_in_file != pos) fail("SimpleFS_seek forward wrong pos");
    
    SimpleFS_write(fh, "ciao", 4);
    
    SimpleFS_close(fh);
    free(root->dcb);
    free(root);
    
    printf(" >> TEST PASSED!\n");
    return 0;
}



