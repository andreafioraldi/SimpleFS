#include <cstdlib>
#include <iostream>
#include <vector>

extern "C" {
#include "simplefs.h"
}

using namespace std;

string colored(string s)
{
    return "\033[0;33m" + s + "\033[0m";
}

void help()
{
    cout << "Avaiable commands:\n";
    cout << "   exit\n";
    cout << "   help\n";
    cout << "   pwd\n";
    cout << "   dir\n";
    cout << "   cd <dirname>\n";
    cout << "   create <filename>\n";
    cout << "   edit <filename>\n";
    cout << "   cat <filename>\n";
    cout << "   hexdump <filename>\n";
    cout << "   del <filename|dirname>\n";
    cout << "   mkdir <dirname>\n";
}

void SimpleFS_shell(DirectoryHandle *root)
{
    vector<DirectoryHandle*> cwd;
    string cmd;
    
    cout << " >>> SimpleFS shell <<<\n";
    cout << "Type 'help' for help.\n\n";
    
    while(1)
    {
        cout << colored("SimpleFS> ");
        getline(cin, cmd);
        cmd.erase(0, cmd.find_first_not_of(" \t\n\r\f\v"));
        cmd.erase(cmd.find_last_not_of(" \t\n\r\f\v") + 1);
        
        if(cmd == "exit")
            exit(0);
        else if(cmd == "help")
            help();
        else if(cmd == "pwd")
        {
            cout << "/";
            for(size_t i = 0; i < cwd.size(); ++i)
                cout << cwd[i]->dcb->fcb.name << "/";
            cout << endl;
        }
        
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
