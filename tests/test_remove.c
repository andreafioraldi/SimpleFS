#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void print_directory_tree(DirectoryHandle *d, int level)
{
    int len = d->dcb->num_entries;
    
    char **names = malloc(len*sizeof(char*));
    SimpleFS_readDir(names, d);
    
    int i, j, r;
    for(i = 0; i < len; ++i) {
        for(j = 0; j < level; ++j)
            printf("==");
    	printf("> %s\n", names[i]);
    	
    	r = SimpleFS_changeDir(d, names[i]);
    	if(r < 0) //not a dir
        {
            free(names[i]);
    	    continue;
	    }
	    
	    print_directory_tree(d, level +1);
	    SimpleFS_changeDir(d, "..");
	    
	    free(names[i]);
    }
    free(names);
}

void fail(char *m)
{
    printf(" >> TEST FAILED - %s\n", m);
    exit(1);
}

int main(int argc, char** argv)
{
    SimpleFS sfs;
    DiskDriver disk;

    DiskDriver_init(&disk, "remove.disk", 1024);
    DirectoryHandle *root = SimpleFS_init(&sfs, &disk);
    
    if(root == NULL || (argc > 1 && !strcmp(argv[1], "format")))
    {
        fprintf(stderr, " >> formatting...\n");
        SimpleFS_format(&sfs);
        root = SimpleFS_init(&sfs, &disk);
        if(argc > 1 && !strcmp(argv[1], "format"))
            return 0;
    }
    
    int r;
    
    SimpleFS_mkDir(root, "folder0");
    SimpleFS_mkDir(root, "folder1");
    
    SimpleFS_changeDir(root, "folder0");
    
    SimpleFS_mkDir(root, "folder2");
    SimpleFS_close(SimpleFS_createFile(root, "file0"));
    SimpleFS_close(SimpleFS_createFile(root, "file1"));
    
    SimpleFS_changeDir(root, "..");
    
    SimpleFS_close(SimpleFS_createFile(root, "file2"));
    
    r = SimpleFS_remove(root, "file2");
    if (r < 0) fail("SimpleFS_remove(root, \"file2\")");
    
    SimpleFS_changeDir(root, "folder1");
    
    SimpleFS_mkDir(root, "folder3");
    SimpleFS_changeDir(root, "folder3");
    
    SimpleFS_close(SimpleFS_createFile(root, "file3"));
    
    SimpleFS_changeDir(root, "..");
    SimpleFS_changeDir(root, "..");
    
    r = SimpleFS_remove(root, "folder0");
    if (r < 0) fail("SimpleFS_remove(root, \"folder0\")");
    r = SimpleFS_remove(root, "folder0"); //yet removed
    if (r >= 0) fail("SimpleFS_remove(root, \"folder0\")");
    
    /*printf("%s   :%d\n", root->dcb->fcb.name, root->dcb->num_entries);
    printf("%d\n", root->dcb->file_blocks[0]);
    printf("%d\n", root->dcb->file_blocks[1]);
    printf("%d\n", root->dcb->file_blocks[2]);
    
    print_directory_tree(root, 0);*/
    
    if(root->directory != root->dcb)
        free(root->directory);
    free(root->dcb);
    free(root);
}



