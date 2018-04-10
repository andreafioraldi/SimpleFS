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
    newfile->header.block_in_disk = new_idx;
    newfile->fcb.directory_block = fb->fcb.block_in_disk;
    newfile->fcb.block_in_disk = new_idx;
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
        new_db.header.block_in_disk = n_block_idx;
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
    if(f->fcb != f->current_block)
        free(f->current_block);
    free(f->fcb);
    free(f);
    return 0;
}


//funzione write che restituisce il numero di byte scritti. Prende come parametro il puntatore al file aperto, il puntatore all'array di byte da scivere e la lunghezza dei dell'array di byte da scrivere
int SimpleFS_write(FileHandle* f, void* data, int size)
{
    //lunghezza blocco dati primo blocco
    int data_len_fb = BLOCK_SIZE - sizeof(FileControlBlock) - sizeof(BlockHeader);
    //lunghezza blocco dati altri blocchi
    int data_len = BLOCK_SIZE - sizeof(BlockHeader);
    
    //byte liberi del blocco dati corrente
    int free_bytes;
    
    //cursore
    int cursor = f->pos_in_file;
    
    // numero di blocco file corrente
    int num_of_block = f->current_block->block_in_file;
    
    // numero di blocco disco dove è mappato il numero di blocco file
    int block_in_disk;
    
    // pntatore asiliario che punta ad array già allocato in memoria dove srivere i byte e che a suo volta dovrà essere scritto sul disco
    
    char *target;
    
    // setto informazioni...
    //se parto dal primo blocco del file
    if (num_of_block == 0)
    {
        // ho tutti i byte del primo blocco tranne quelli già scritti
        free_bytes = data_len_fb - cursor;
        //punto target
        target = ((FirstFileBlock*)f->current_block)->data;
        
        //dico che la lunghezza del blocco dati in questo caso è quella del primo blocco
        data_len = data_len_fb;
        
        // setto l'indice del blocco disco dove è mappato il primo blocco del file
        block_in_disk = f->fcb->fcb.block_in_disk;

    }
    //se non parto dal primo blocco del file
    else 
    {
        // ho tutti i byte del blocco tranne quelli già scritti
        free_bytes = data_len - (cursor - data_len_fb - ((num_of_block - 1) * data_len));
        
        //punto target
        target = ((FileBlock*)f->current_block)->data;
        
        //data len è già settata all'inizio
        
        // setto l'indice del blocco disco dove è mappato il primo blocco del file
        block_in_disk = f->current_block->block_in_disk; 
    }
    
    //scrivo...
    // se ho abbastanza spazio nel blocco data
    if (size<= free_bytes) 
    {
        //scrivo in target la parola
        memcpy(target + (data_len - free_bytes), data, size);
        
        //scrivo sul blocco corrente target
        DiskDriver_writeBlock(f->sfs->disk, f->current_block, block_in_disk);
        
        //aggiorni cursore
        f->pos_in_file += size;
        
        f->fcb->fcb.size_in_bytes += size;
        DiskDriver_writeBlock(f-> sfs->disk, f->fcb, f->fcb->header.block_in_disk);
        
        return size;
    }
    // se non ho abbastanza spazio nel blocco data ma il blocco successivo è già stato creato
    else if (f->current_block->next_block != -1)
    {
        //scrivo in target la parte di parola che posso scrivere
        memcpy(target + (data_len - free_bytes), data, free_bytes);
        
        //scrivo sul blocco corrente target
        DiskDriver_writeBlock(f->sfs->disk, f->current_block, block_in_disk);
        
        //aggiorni cursore
        f->pos_in_file += free_bytes;
        
        // alloco uno spazio in meoria su cui copiare il prossimo record dal disco
        FileBlock * tmp = calloc(1, sizeof(FileBlock));
        
        //copio blocco dal disco (il record susccessivo a quello corrente)
        DiskDriver_readBlock(f->sfs->disk, tmp, f->current_block->next_block);
        
        //scorro la lista dei blocchi (quello corrente è quello appena caricato)
        f->current_block = (BlockHeader*)tmp;
        
        f->fcb->fcb.size_in_bytes += free_bytes;
        f->fcb->fcb.size_in_blocks += 1;
        
        //richiamo wrute tornando al caso iniziale
        return free_bytes + SimpleFS_write(f, data + free_bytes, size-free_bytes);
    }
    // se non ho abbastanza spazio nel blocco data ed il blocco successivo non è ancora stato creato
    else 
    {
        //prendo l'indice del nuovo blocco
        int tmp_block_in_disk =  DiskDriver_getFreeBlock(f->sfs->disk, f->sfs->disk->header->first_free_block);
        
        //array che scrivo sul blocco disco
        FileBlock tmp = {0};
        tmp.header.previous_block = block_in_disk;
        tmp.header.next_block = -1;
        tmp.header.block_in_file = num_of_block +1;
        tmp.header.block_in_disk = tmp_block_in_disk;
        
        //scrivo l'array sul blocco disco
        DiskDriver_writeBlock(f->sfs->disk, &tmp, tmp_block_in_disk);
        
        //aggiorno la lista dei blocchi dicendo che ho allocato un nuovo blocco per il file
        f->current_block->next_block = tmp_block_in_disk; 
        
        //richiamo tornanod al caso iniziale    
        return SimpleFS_write(f, data, size);
    }
}





