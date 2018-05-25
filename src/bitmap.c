#include "bitmap.h"

extern inline BitMapEntryKey BitMap_blockToIndex(int num);

extern inline int BitMap_indexToBlock(int entry, uint8_t bit_num);

extern inline int BitMap_get(BitMap* bmap, int start, int status);

extern inline int BitMap_lookup(BitMap* bmap, int pos);

extern inline int BitMap_set(BitMap* bmap, int pos, int status);


