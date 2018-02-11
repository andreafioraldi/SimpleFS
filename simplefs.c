#include "simplefs.h"
#include <stdlib.h>
#include <strings.h>

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
    
    FirstDirectoryBlock root = {0};
    root.header.previous_block = root.header.next_block = -1;
    root.fcb.directory_block = -1;
    strcpy(root.fcb.name, "/");
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
    
    //check if a file already exists
    FirstFileBlock tmp;
    int i, pos = 0;
    
    for(i = 0; i < fb->num_entries && i < fb_data_len; ++i)
    {
        DiskDriver_readBlock(d->sfs->disk, &tmp, fb->file_blocks[i]);
        if(!strncmp(tmp.fcb.name, filename, FILENAME_MAX_LEN))
            return NULL;
    }
    
    int block_idx, one_block = 1;
    DirectoryBlock db;
    
    if(i < fb->num_entries)
    {
        one_block = 0;
        block_idx = fb->header->next_block;
        
        while(i < fb->num_entries)
        {
            DiskDriver_readBlock(d->sfs->disk, &db, block_idx);
            fb_data_len += (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int);
            
            for(; i < fb->num_entries && i < fb_data_len; ++i)
            {
                DiskDriver_readBlock(d->sfs->disk, &tmp, db->file_blocks[i]);
                if(!strncmp(tmp.fcb.name, filename, FILENAME_MAX_LEN))
                    return NULL;
            }
            
            ++pos;
            
            if(i < fb->num_entries)
                block_idx = db->header->next_block;
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
    if(i < fb_data_len)
    {
        if(one_block)
            fb->file_blocks[i] = new_idx;
        else
        {
            db.file_blocks[i] = new_idx;
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
        new_db.previous_block = block_idx;
        new_db.next_block = -1;
        new_db.block_in_file = pos +1;
        new_db.file_blocks[0] = new_idx;
        
        DiskDriver_writeBlock(d->sfs->disk, &new_db, n_block_idx);
        
        //update next_block with new dir block index
        db.header.next_block = n_block_idx;
        DiskDriver_writeBlock(d->sfs->disk, &db, block_idx);
    }
    
    ++fb->num_entries;
    DiskDriver_writeBlock(d->sfs->disk, newfile, new_idx);
    
    //update fb
    DiskDriver_writeBlock(d->sfs->disk, fb, fb->fcb.block_in_disk);
}











