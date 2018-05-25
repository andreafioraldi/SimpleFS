#include "disk_driver.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

void DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks)
{
    int ret;
    int fd;
     
    int bmap_entries = num_blocks / 8;
    if(num_blocks % 8)
        ++bmap_entries;
    
    int exists = access(filename, F_OK) != -1;
    int space = sizeof(DiskHeader) + bmap_entries + num_blocks * BLOCK_SIZE;
       
    if(exists)
    {
        fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    }
    else 
    {
        fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        
        ret = lseek(fd, space -1, SEEK_SET);
        if (ret == -1)
        {
	        fprintf(stderr, "error: cannot allocate new disk file\n");
            exit(EXIT_FAILURE);
        }

        //write to the last byte to set the size
        ret = write(fd, "", 1);
        if (ret == -1)
        {
	        fprintf(stderr, "error: cannot write to the new disk file\n");
            exit(EXIT_FAILURE);
        }
        
        lseek(fd, 0, SEEK_SET);
    }
    
    DiskHeader* mem = mmap(0, space, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    disk->header = mem;
    disk->bitmap_data = (char*)mem + sizeof(DiskHeader);
    disk->fd = fd;
    
    if(exists)
    {
        if(mem->num_blocks != num_blocks)
        {
            fprintf(stderr, "error: the number of blocks in the disk are not equal to the requested number of blocks");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        mem->num_blocks = num_blocks;
        mem->bitmap_blocks = num_blocks;
        mem->bitmap_entries = bmap_entries;
        
        mem->free_blocks = num_blocks;
        mem->first_free_block = 0;
        
        bzero(disk->bitmap_data, bmap_entries);
    }
}



int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num)
{
    BitMap bmap;
    bmap.num_bits = disk->header->bitmap_blocks;
    bmap.entries = disk->bitmap_data;
    
    if(!BitMap_lookup(&bmap, block_num)) //check it the block is free
        return -1;
    
    char* mem = disk->bitmap_data + disk->header->bitmap_entries;
    memcpy(dest, mem + block_num * BLOCK_SIZE, BLOCK_SIZE);
    return 0;
}


int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num)
{
    if(block_num >= disk->header->num_blocks) //check overflow
        return -1;
    
    BitMap bmap;
    bmap.num_bits = disk->header->bitmap_blocks;
    bmap.entries = disk->bitmap_data;
    
    if(block_num == disk->header->first_free_block)
        disk->header->first_free_block = DiskDriver_getFreeBlock(disk, block_num +1);
    
    if(BitMap_set(&bmap, block_num, 1) == -1)
        return -1;
    
    char* mem = disk->bitmap_data + disk->header->bitmap_entries;
    memcpy(mem + block_num * BLOCK_SIZE, src, BLOCK_SIZE);
    
    return 0;
}


int DiskDriver_freeBlock(DiskDriver* disk, int block_num)
{
    if(block_num >= disk->header->num_blocks) //check overflow
        return -1;
    
    BitMap bmap;
    bmap.num_bits = disk->header->bitmap_blocks;
    bmap.entries = disk->bitmap_data;
    
    if(!BitMap_lookup(&bmap, block_num)) //double free
        return -1;
    
    if(BitMap_set(&bmap, block_num, 0) == -1)
        return -1;
    
    if(block_num < disk->header->first_free_block) //update first_free_block
        disk->header->first_free_block = block_num;
    
    ++disk->header->free_blocks;
    
    return 0;
}


int DiskDriver_getFreeBlock(DiskDriver* disk, int start)
{
    if(start >= disk->header->num_blocks) //check overflow
        return -1;
    
    BitMap bmap;
    bmap.num_bits = disk->header->bitmap_blocks;
    bmap.entries = disk->bitmap_data;
    
    return BitMap_get(&bmap, start, 0);
}


int DiskDriver_flush(DiskDriver* disk)
{
    printf("DiskDriver_flush NOT IMPLEMENTED\n");
    exit(1);
}


