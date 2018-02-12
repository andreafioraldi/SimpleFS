#include <cstdlib>
#include <iostream>

extern "C" {
#include "simplefs.h"
}

using namespace std;

void SimpleFS_shell(DirectoryHandle *root)
{
    string cmd;
    while(1)
    {
        cout << "SimpleFS> ";
        getline(cin, cmd);
        if(cmd == "exit")
            exit(0);
        cout << cmd << endl;
    }
}

int main(int argc, char **argv)
{
    if(argc < 3)
    {
        cerr << "usage: simplefs_shell <disk-file> <num-blocks>\n";
        return 1;
    }
    
    SimpleFS sfs;
    DiskDriver disk;
    
    DiskDriver_init(&disk, argv[1], atoi(argv[2]));
    DirectoryHandle *root = SimpleFS_init(&sfs, &disk);
    
    if(root == NULL)
    {
        cout << "formatting...\n";
        SimpleFS_format(&sfs);
        root = SimpleFS_init(&sfs, &disk);
    }
    
    SimpleFS_shell(root);
}
