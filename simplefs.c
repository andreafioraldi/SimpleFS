#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    /*
    int i;
    for(i=0;i< 512;++i) {
        printf("%02hhX ", ((char*)fdb)[i]);
        if((i+1)%16==0)printf("\n");
    }
    printf("\n");
    */
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
    
    printf("CREATE FILE %s\n", filename);
    printf("entries: %d\n", fb->num_entries);
    
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
        
        while(total_idx < fb->num_entries)
        {
            DiskDriver_readBlock(d->sfs->disk, &db, block_idx);
            fb_data_len = (BLOCK_SIZE-sizeof(BlockHeader))/sizeof(int);
            
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
    
    printf("CREATION OF %s SUCCESSFUL\n", filename);
    
    return handle;
}



