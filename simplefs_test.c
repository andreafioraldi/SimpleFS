#include "simplefs.h"
#include <stdio.h>
int main(int agc, char** argv) {
  printf("FirstBlock size %ld\n", sizeof(FirstFileBlock));
  printf("DataBlock size %ld\n", sizeof(FileBlock));
  printf("FirstDirectoryBlock size %ld\n", sizeof(FirstDirectoryBlock));
  printf("DirectoryBlock size %ld\n", sizeof(DirectoryBlock));
  
  SimpleFS sfs;
  DiskDriver disk;
  
  DiskDriver_init(&disk, "test.disk", 32);
  
  SimpleFS_init(&sfs, &disk);
  SimpleFS_format(&sfs);
  
  
  
}
