#ifndef DEV_H
#define DEV_H
extern FILE *SwapFile;
extern FILE *FSFile;
extern int SwapCount[SWAP_SIZE];
extern fs_block_t FSBlock[FS_SIZE];

void mount_dev();
void unmount_dev();
int empty_swap_block();
void swap_in(byte_t *, int);
void load_block(int, int);
int swap_out(byte_t*);
#endif
