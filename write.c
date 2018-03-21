#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int SimpleFS_write_aux (FileHandle* f, void* data, int size, int index_block_disk);

int SimpleFS_write(FileHandle* f, void* data, int size)
{
	/*
	 * Informazioni base:
	 * 		-numero byte liberi nel blocco dati del file
	 * 		-numero blocco del disco su cui è mappato il blocco del file
	 * 
	 *Se mi trovo all'inizio del file
	 * 		-se scrivo sul file tale che non supero la lunghezza del blocco corrente;
	 * 			-numero blocco nel disco = file handler -> first file block -> file control block -> block in the disk;
	 * 		-se scrivo sul file superando la lunghezza del blocco corrente;
	 * 			-numero blocco nel disco = DiskDriver_getFreeBlock;
	 * 
	 * 
	 */
	 
	 //array su cui scrivere il testo

	 
	 //Se mi trovo all'inizio del file
	 if (f -> current_block -> block_in_file == 0)
	 {
		 // se scrivo sul file tale che non supero la lunghezza del blocco corrente;
		 
		 // bytes liberi sul blocco data
		 int free_bytes_in_block_data = BLOCK_SIZE-sizeof(FileControlBlock) - sizeof(BlockHeader) - f -> pos_in_file; //se file è vuoto cursore == 0
		 
		 if (size < free_bytes_in_block_data) 
		 {
			// scrivo in memoria sul blocco dati del file il testo della write usnado memcpy (destinzazione, fonte, byte_da_scrivere)
			// destinazione = data + pos_in_file
			// fonte = data
			// byte_da_scrivere = size
			memcpy(((FirstFileBlock*)f->current_block)->data + f -> pos_in_file, data, size);
			
			//scrivo nel disco il blocco data scritto in memoria
			DiskDriver_writeBlock(f -> sfs -> disk, ((FirstFileBlock*)f->current_block)->data , f -> fcb -> fcb.block_in_disk);
			
			//aggiorno cursore
			f -> pos_in_file += size;
			
			return size;
			
		}
		// se scrivo sul file superando la lunghezza del blocco corrente;
		else 
		{
			// scrivo in memoria sul blocco dati del file il testo della write che posso scrivere usnado memcpy (destinzazione, fonte, byte_da_scrivere)
			// destinazione = data + pos_in_file
			// fonte = data
			// byte_da_scrivere = bytes_liberi = lunghezza blocco dati - pos_in_file
			memcpy(((FirstFileBlock*)f->current_block)->data + f -> pos_in_file, data, free_bytes_in_block_data);
			
			//scrivo nel disco il blocco data scritto in memoria
			DiskDriver_writeBlock(f -> sfs -> disk, ((FirstFileBlock*)f->current_block)->data , f -> fcb -> fcb.block_in_disk);
			
			// indice sul disco del prossimo blocco del file
			int index_disk = DiskDriver_getFreeBlock(f->sfs->disk, f->sfs->disk->header->first_free_block);
			
			//alloco uno spazio di memoria per allocare un blocco
			FileBlock * tmp = calloc(1, sizeof(FileBlock));
			
			//carico dal disco il blocco in memoria
			DiskDriver_readBlock(f->sfs->disk, tmp, index_disk);
			
			//scorro la lista dei blocchi del file
			f->current_block = (BlockHeader*)tmp;
			
			//aggiorno numero del blocco del file
			f -> current_block -> block_in_file += 1;
			
			//ritorno bytes scritti = bytes liberi + richiamo la write scorrendo l'array che devo scrivere e diminuned i byte da srcivere
			// la write successiva andrà subito nell'else del primo if, poichè f -> current_block -> block_in_file != 0
			return free_bytes_in_block_data + SimpleFS_write_aux (f, data + free_bytes_in_block_data, size -free_bytes_in_block_data, index);	
			
		}
	 }
}

int SimpleFS_write_aux (FileHandle* f, void* data, int size, int index_block_disk)
{
	// sarebbe la write solo che ho l'indice del blocco del disco dove devo scrivere
}
	
