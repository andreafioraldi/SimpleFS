#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//useful in debugging
void SimpleFS_hexdump(char *title, void* ptr, int size)
{
    printf("------------------------------------------------------\n");
    printf("      HEXDUMP: %s\n", title);
    printf("------------------------------------------------------\n");
    
    int i;
    printf("   0: ");
    for(i=0; i< size; ++i)
    {
        printf("%02hhX ", ((char*)ptr)[i]);
        if( (i+1)%16==0 && i < size-1)
            printf("\n%4d: ", i+1);
    }
    printf("\n------------------------------------------------------\n\n");
}


DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk)
{
    fs->disk = disk;
    
    FirstDirectoryBlock *fdb = malloc(BLOCK_SIZE);
    int r = DiskDriver_readBlock(disk, fdb, 0);
    if(r == -1)
    {
        free(fdb);
        return NULL;
    }
    
    //SimpleFS_hexdump("SimpleFS_init", fdb, BLOCK_SIZE);
    
    DirectoryHandle *dh = malloc(sizeof(DirectoryHandle));
    dh->sfs = fs;
    dh->dcb = fdb;
    dh->directory = NULL;
    dh->current_block = &fdb->header;
    dh->pos_in_dir = dh->pos_in_block = 0;
    
    return dh;
}


void SimpleFS_format(SimpleFS* fs)
{
    int bmp_size = fs->disk->header->bitmap_entries;
    bzero(fs->disk->bitmap_data, bmp_size);
    fs->disk->header->free_blocks = fs->disk->header->num_blocks;
    fs->disk->header->first_free_block = 0;
    
    //create root folder
    FirstDirectoryBlock root = {0};
    root.header.previous_block = root.header.next_block = -1;
    root.fcb.directory_block = -1;
    strcpy(root.fcb.name, "/");
    root.fcb.size_in_bytes = BLOCK_SIZE;
    root.fcb.size_in_blocks = 1;
    root.fcb.is_dir = 1;
    
    DiskDriver_writeBlock(fs->disk, &root, 0);
}


FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename)
{
    FirstDirectoryBlock *fb = d->dcb;
    int fb_data_len = (BLOCK_SIZE
		   -sizeof(BlockHeader)
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);
    
    //printf("BEGIN CREATE FILE %s\n", filename);

    //check if a file already exists
    FirstFileBlock tmp;
    int idx = 0;
    
    for(; idx < fb->num_entries && idx < fb_data_len; ++idx)
    {
        DiskDriver_readBlock(d->sfs->disk, &tmp, fb->file_blocks[idx]);
        
        //printf("file %d: %s\n", idx, tmp.fcb.name);
        
        if(!strncmp(tmp.fcb.name, filename, FILENAME_MAX_LEN))
            return NULL;
    }
    
    int total_idx = idx;
    int block_idx = fb->fcb.block_in_disk;
    int one_block = 1;
    
    DirectoryBlock db;
        
    if(idx < fb->num_entries)
    {
        one_block = 0;
        block_idx = fb->header.next_block;
        fb_data_len = (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int);
        
        while(total_idx < fb->num_entries)
        {
            DiskDriver_readBlock(d->sfs->disk, &db, block_idx);
            
            idx = 0;
            for(; total_idx < fb->num_entries && idx < fb_data_len; ++idx, ++total_idx)
            {
                DiskDriver_readBlock(d->sfs->disk, &tmp, db.file_blocks[idx]);
                if(!strncmp(tmp.fcb.name, filename, FILENAME_MAX_LEN))
                    return NULL;
            }

            if(total_idx < fb->num_entries)
                block_idx = db.header.next_block;
        }
    }
    
    int new_idx = DiskDriver_getFreeBlock(d->sfs->disk, d->sfs->disk->header->first_free_block);
    if(new_idx == -1)
        return NULL;
    
    FirstFileBlock* newfile = calloc(1, sizeof(FirstFileBlock));
    
    newfile->header.previous_block = newfile->header.next_block = -1;
    newfile->fcb.directory_block = fb->fcb.block_in_disk;
    strncpy(newfile->fcb.name, filename, FILENAME_MAX_LEN);
    newfile->fcb.size_in_blocks = 1;

    //do not add block to dir
    if(idx < fb_data_len)
    {
        if(one_block)
            fb->file_blocks[idx] = new_idx;
        else
        {
            db.file_blocks[idx] = new_idx;
            //update last dir block
            DiskDriver_writeBlock(d->sfs->disk, &db, block_idx);
        }
    }
    else
    {
        int n_block_idx = DiskDriver_getFreeBlock(d->sfs->disk, new_idx +1);
        if(n_block_idx == -1)
        {
            free(newfile);
            return NULL;
        }
        
        DirectoryBlock new_db = {0};
        new_db.header.previous_block = block_idx;
        new_db.header.next_block = -1;
        new_db.header.block_in_file = total_idx / fb->num_entries;
        new_db.file_blocks[0] = new_idx;
        
        DiskDriver_writeBlock(d->sfs->disk, &new_db, n_block_idx);
        
         //update next_block with new dir block index
        if(one_block)
            fb->header.next_block = n_block_idx;
        else
        {
            db.header.next_block = n_block_idx;
            DiskDriver_writeBlock(d->sfs->disk, &db, block_idx);
        }
          
        fb->fcb.size_in_bytes += BLOCK_SIZE;
        fb->fcb.size_in_blocks += 1;
    }
    
    DiskDriver_writeBlock(d->sfs->disk, newfile, new_idx);
    
    //update fb
    ++fb->num_entries;
    DiskDriver_writeBlock(d->sfs->disk, fb, fb->fcb.block_in_disk);
    
    FileHandle *handle = malloc(sizeof(FileHandle));
    handle->sfs = d->sfs;
    handle->fcb = newfile;
    handle->directory = fb;
    handle->current_block = (BlockHeader*)newfile;
    handle->pos_in_file = 0;
    
    //printf("CREATION OF %s SUCCESSFUL\n", filename);
    
    return handle;
}


int SimpleFS_readDir(char** names, DirectoryHandle* d)
{
    FirstDirectoryBlock *fb = d->dcb;
    int fb_data_len = (BLOCK_SIZE
		   -sizeof(BlockHeader)
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);

    //check if a file already exists
    FirstFileBlock tmp;
    int idx = 0;
    
    for(; idx < fb->num_entries && idx < fb_data_len; ++idx)
    {
        DiskDriver_readBlock(d->sfs->disk, &tmp, fb->file_blocks[idx]);
        names[idx] = strndup(tmp.fcb.name, FILENAME_MAX_LEN);
    }
    
    int total_idx = idx;
    int block_idx = fb->fcb.block_in_disk;

    DirectoryBlock db;
        
    if(idx < fb->num_entries)
    {
        block_idx = fb->header.next_block;
        fb_data_len = (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int);
        
        while(total_idx < fb->num_entries)
        {
            DiskDriver_readBlock(d->sfs->disk, &db, block_idx);
            
            idx = 0;
            for(; total_idx < fb->num_entries && idx < fb_data_len; ++idx, ++total_idx)
            {
                DiskDriver_readBlock(d->sfs->disk, &tmp, db.file_blocks[idx]);
                names[total_idx] = strndup(tmp.fcb.name, FILENAME_MAX_LEN);
            }
        }
    }
    
    return total_idx;
}



FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename)
{
    FirstDirectoryBlock *fb = d->dcb;
    int fb_data_len = (BLOCK_SIZE
		   -sizeof(BlockHeader)
		   -sizeof(FileControlBlock)
		    -sizeof(int))/sizeof(int);

    //check if a file already exists
    int found = 0;
    FirstFileBlock *readed = malloc(sizeof(FirstFileBlock));
    int idx = 0;
    
    for(; idx < fb->num_entries && idx < fb_data_len; ++idx)
    {
        DiskDriver_readBlock(d->sfs->disk, readed, fb->file_blocks[idx]);
        if(!strncmp(readed->fcb.name, filename, FILENAME_MAX_LEN))
        {
            found = 1;
            break;
        }
    }
    
    int total_idx = idx;
    int block_idx = fb->fcb.block_in_disk;

    DirectoryBlock db;
        
    if(!found && idx < fb->num_entries)
    {
        block_idx = fb->header.next_block;
        fb_data_len = (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int);
        
        while(total_idx < fb->num_entries)
        {
            DiskDriver_readBlock(d->sfs->disk, &db, block_idx);
            
            idx = 0;
            for(; total_idx < fb->num_entries && idx < fb_data_len; ++idx, ++total_idx)
            {
                DiskDriver_readBlock(d->sfs->disk, readed, db.file_blocks[idx]);
                if(!strncmp(readed->fcb.name, filename, FILENAME_MAX_LEN))
                {
                    found = 1;
                    break;
                }
            }
        }
    }
    
    if(!found)
    {
        free(readed);
        return NULL;
    }
    
    FileHandle *handle = malloc(sizeof(FileHandle));
    handle->sfs = d->sfs;
    handle->fcb = readed;
    handle->directory = fb;
    handle->current_block = (BlockHeader*)readed;
    handle->pos_in_file = 0;
    
    return handle;
}


int SimpleFS_close(FileHandle* f)
{
    int r = 0;
    if(f->fcb != f->current_block)
        r = free(f->current_block);
    r |= free(f->fcb);
    r |= free(f);
    return r;
}


//funzione write che restituisce il numero di byte scritti. Prende come parametro il puntatore al file aperto, il puntatore ai byte da scivere e la lunghezza dei byte da scrivere
int SimpleFS_write(FileHandle* f, void* data, int size)
{
    
    /*
    typedef struct {
            SimpleFS* sfs;                   // pointer to memory file system structure
            FirstFileBlock* fcb;             // pointer to the first block of the file(read it)
            FirstDirectoryBlock* directory;  // pointer to the directory where the file is stored
            BlockHeader* current_block;      // current block in the file
            int pos_in_file;                 // position of the cursor
        } FileHandle;
        
        typedef struct {
                int previous_block; // chained list (previous block)
                int next_block;     // chained list (next_block)
                int block_in_file; // position in the file, if 0 we have a file control block
        } BlockHeader;
        
        typedef struct {
            BlockHeader header;
            char  data[BLOCK_SIZE-sizeof(BlockHeader)];
        } FileBlock;

    */
    //puntatore ai byte da scrivere
    char* block_data;
    //lunghezza dei byte da scrivere
    int data_len;

    
    //il file è diviso in record. il primo record del file contiente una porzione che indica il controllo del file una che rappresnta l'hadler e una che rappresenta la porzione di file su cui scrivere
    // dal primo in poi i record sono formati solo da handler e blocco file. Quindi prima di scrivere devo vedere se il puntatore del file aperto punta al primo record o non
    
    //se la posizione del primo record del file è quella puntata dal puntatore del file
    if(f->fcb == f->current_block)
    {
        //ottengo un array di lunghezza di un blocco file e la sua lunghezza
        block_data = f->fcb->data;
        data_len = BLOCK_SIZE - sizeof(FileControlBlock) - sizeof(BlockHeader);
    }
    //altrimenti
    else
    {
        //ottengo un array di lunghezza di un blocco file e la sua lunghezza
        block_data = ((FileBlock*)f->current_block)->data;
        data_len = BLOCK_SIZE - sizeof(BlockHeader);
    }
    
    //quando scrivo ho 2 casi: 1) in cui la lunghezza di byte da scrivere è minore della lunghezza dei vari blocchi del file quindi non devo allocare nessun blocco ma solo scorrerli mentre l'altro devo allocarli
    
    
    
    
    int s = data_len;
    // se i byte da scrivere hanno una lunghezza minore del recrod
    if(size < data_len)
        //mi segno effetivamente quanti byte devo scrivere
        s = size;
    //copia i byte partendo dalla prima cella puntata in data nell'array block_data fermandosi alla cella di posizione s-1
    memcpy(data, block_data, s);
    
    if(size < data_len)
        return size;
        
    
    // se i byte da scrivere hanno una lunghezza maggiore del recrod
    if(size > data_len)
    {
    
        
        
    
        //se ho ancora un indica al prossimo blocco
        if (f->current block->next block != NULL) 
        {
            // alloco uno spazio in meoria su cui copiare il record dal disco
            FileBlock* file_block = (FileBlock*) malloc (sizeof(FileBlock));
            //copio record dal disco (il record susccessivo a quello corrente)
            DiskDriver_readBlock(f->sfs->disk, (void*) file_block, f->current block->next block);
            //scorro la lista dei record
            f->current block = file_block;
            //richiamo la funzione
            f -> pos_in_file += data_len;
            
            return SimpleFS_write(f, data + data_len, size-data_len) + data_len;
        
        }
        
        else 
        {
          int index =  DiskDriver_getFreeBlock(f->sfs->disk, f -> fcb);
          FileBlock* file_block = (FileBlock*) malloc (sizeof(FileBlock));
          int DiskDriver_writeBlock(f->sfs->disk, file_block, index);
          f->current block->next block = index;
          
          return SimpleFS_write(f, data, size) + 0;
          
          
        }
    
    
    
    
    
        
        
    }
    

}





