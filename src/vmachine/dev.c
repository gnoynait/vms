#include"vmm.h"
#include"dev.h"
#include "vmachine.h"
/* the devices */
FILE *SwapFile;
FILE *FSFile;

int SwapCount[SWAP_SIZE];
fs_block_t FSBlock[FS_SIZE];

/***************************************************
 * init the devices whitch are actually files on the 
 * disk.
 **************************************************/
void mount_dev(){
	int i, j;
	if((SwapFile = fopen(SWAP_FNAME, "w")) == NULL){
		error("can't open swap file.");
	}
	if((FSFile = fopen(FS_FNAME, "w")) == NULL){
		error("can't open swap file.");
	}

	for(i = 0; i < SWAP_SIZE; i++)
		SwapCount[i] = 0;
	for(i = 0; i < FS_SIZE; i++){
		FSBlock[i].count = 0;
		FSBlock[i].page_ID = -1;
	}
}

/************************************************
 * unmount the devices: the virtual device files.
 ***********************************************/
void unmount_dev(){
	fclose(SwapFile);
	fclose(FSFile);
}

/************************************************
 * return the id of the available block int the 
 * swap device, return -1 if none available.
 ************************************************/
int empty_swap_block(){
	int i;
	for(i = 0; i < SWAP_SIZE; i++){
		if(SwapCount[i] == 0)
			return i;
	}
	return -1;
}

/***********************************************
 * read swap to memory, and minus its count by 1
 ***********************************************/
void swap_in(byte_t *mem, int block){
	printf("swaping in.\n");
	fseek(SwapFile, block * PAGE_SIZE, SEEK_SET);
	fread(mem, PAGE_SIZE, 1, SwapFile);
	SwapCount[block]--;
}
    
void load_block(int mem, int block){
	assert(mem != -1);
	printf("loading block.\n");
	fseek(FSFile, block * PAGE_SIZE, SEEK_SET);
	fread(&Memory[mem * PAGE_SIZE], PAGE_SIZE, 1, FSFile);
	FSBlock[block].count++;
	FSBlock[block].page_ID = mem;
}
/******************************************************
 * swap the PAGE_SIZE data from mem to a swap area.
 * return the swap block number.
 *****************************************************/
int swap_out(byte_t *mem){
	int block = empty_swap_block();
	printf("swapping out %d.\n", block);
	if(block < -1)
		error("no swap space.");
	fseek(SwapFile, block * PAGE_SIZE, SEEK_SET);
	fwrite(mem, PAGE_SIZE, 1, SwapFile);
	SwapCount[block]--;
	return block;
}
