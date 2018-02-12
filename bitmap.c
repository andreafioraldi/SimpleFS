#include "bitmap.h"

BitMapEntryKey BitMap_blockToIndex(int num)
{
    BitMapEntryKey e;
    e.entry_num = num >> 3;
    e.bit_num = num & 0x7;
    return e;
}

int BitMap_indexToBlock(int entry, uint8_t bit_num)
{
    return (entry << 3) | (bit_num & 0x7);
}

int BitMap_get(BitMap* bmap, int start, int status)
{
    int i = start;
    while(i < bmap->num_bits)
    {
        BitMapEntryKey e = BitMap_blockToIndex(i);
        int s = bmap->entries[e.entry_num] >> e.bit_num & 0x1;
        
        if(s == status)
            return i;
        ++i;
    }
    return -1;
}

int BitMap_lookup(BitMap* bmap, int pos)
{
    if(pos >= bmap->num_bits)
        return -1;
    
    BitMapEntryKey e = BitMap_blockToIndex(pos);
    return bmap->entries[e.entry_num] >> e.bit_num & 0x1;
}

int BitMap_set(BitMap* bmap, int pos, int status)
{
    if(pos >= bmap->num_bits)
        return -1;
    
    BitMapEntryKey e = BitMap_blockToIndex(pos);
    bmap->entries[e.entry_num] &= ~(1 << e.bit_num);
    bmap->entries[e.entry_num] |= status << e.bit_num;
    return pos;
}


