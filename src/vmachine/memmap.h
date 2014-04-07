#ifndef MEMMAP_H
#define MEMMAP_H

extern page_t MemMap[MMAP_SIZE];

void init_memmap();
void display_memmap();
void page_in(pte_t*);
void page_out(int);
int empty_page();
void memmap_refresh();

#endif
