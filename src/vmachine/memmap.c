#include"vmm.h"
#include"memmap.h"
#include"dev.h"
#include"vmachine.h"
// control the pages
page_t MemMap[MMAP_SIZE];

void init_memmap(){
	int i;
	for(i = 0; i< MMAP_SIZE; i++){
		MemMap[i].accessed = FALSE;
		MemMap[i].freq = 0x00;
		//no refer to the inited page yet.
		MemMap[i].lock = -1;
	
		MemMap[i].ref_pte = NULL;
	}
}

void display_memmap(){
	int i;
	fprintf(stdout, "\nMemMap info\n");
	fprintf(stdout, "id     lock  accessed  freq \n");
	for(i = 0; i < MMAP_SIZE; i++) {
		if (MemMap[i].lock == -1)
			continue;
		fprintf(stdout, "%2x      %2d       %d      %d\n",
			i, MemMap[i].lock, MemMap[i].accessed, MemMap[i].freq);
	}
}

/**
 * most high level procedure for dealing with page 
 * fault. it will load the page pointed by the pte
 * from swap, fs.
 */
void page_in(pte_t *pte){
	int empty_pg;
	fprintf(stdout, "page_in: vpage:%x, swapped:%d, valid:%d, page_ID:%x\n",
		pte->page_ID, pte->swapped, pte->valid, pte->page_ID);
	if(pte->swapped == TRUE){
		fprintf(stdout, "page_in: from swap\n");
		empty_pg = empty_page();

		swap_in(&Memory[empty_pg], pte->page_ID);
		pte->swapped == FALSE;
		/*since shared page won't be swapped out */
		pte->mode &= (~MODE_C);
		pte->page_ID = empty_pg;
					
		MemMap[empty_pg].lock = 0;
		MemMap[empty_pg].ref_pte = pte;
		assert(pte != NULL);
	} else {
		//assume all pte refer to the same page have the same mode.
		//if the pte is readonly, other is readonly, too.
		if (pte->page_ID == -1) { /* load null page */
			empty_pg = empty_page();
			fprintf(stdout, "page_in: load null "
				"to empty ppage:%x.\n",
				empty_pg);
			pte->page_ID = empty_pg;
			pte->mode = MODE_RW;
			pte->mode &= (~MODE_C);
			MemMap[empty_pg].lock = 0;
			MemMap[empty_pg].ref_pte = pte;
			MemMap[empty_pg].block = -1;
			assert(pte != NULL);
		} else if (FSBlock[pte->page_ID].count > 0) {
			pte->page_ID = FSBlock[pte->page_ID].page_ID;
			pte->mode |= MODE_C;
			MemMap[pte->page_ID].ref_pte->mode |= MODE_C;
			/* PTE's mode is not changed. */
			fprintf(stdout, "page_in: share:%x\n", pte->page_ID);
			MemMap[pte->page_ID].lock++;
			MemMap[pte->page_ID].ref_pte = pte;
			assert(pte != NULL);
			FSBlock[pte->page_ID].count++;
		} else {
			empty_pg = empty_page();
			fprintf(stdout, "page_in: load to  empty ppage:%x.\n",
				empty_pg);
			load_block(empty_pg, pte->page_ID);
			MemMap[empty_pg].block = pte->page_ID;
			pte->page_ID = empty_pg;
			pte->mode &= (~MODE_C);
			/* PTE's mode not change. */
			pte->edited = FALSE;
			MemMap[empty_pg].lock = 0;
			MemMap[empty_pg].ref_pte = pte;
			assert(pte != NULL);
		}
	}
	pte->valid = TRUE;
}

/**
 * Swap page out.
 * if the page hasn't been edited, just redirect the pte to the block.
 */
void page_out(int page_id) 
{
	page_t *page = MemMap + page_id;
	pte_t *pte = page->ref_pte;
	int swap_i;
	printf("page out %x\n", page_id);
	if (pte->edited == FALSE) {
		pte->page_ID = page->block;
		pte->swapped = FALSE;
	} else {
		pte->swapped = TRUE;
		swap_i = swap_out(&Memory[page_id * PAGE_SIZE]);
		pte->page_ID = swap_i;
	}
	page->lock = -1;
	page->accessed = FALSE;
	page->freq = 0x00;
	page->ref_pte = NULL;
	pte->valid = FALSE;
}


/**
 * get a usable page frame, return its index.
 * if there is a page that is empty, return 
 * it directly. if not , exchage one.
 * notice: it won't return a shared page.
 */
int empty_page(){
	int min = 0;
	int i;
	for(i = 0; i < MMAP_SIZE; i ++){
		//shared page, just pass it.
		if(MemMap[i].lock > 1)
			continue;
		//unused page
		if (MemMap[i].lock == -1){
			MemMap[i].accessed = FALSE;
			MemMap[i].freq = 0x00;
			return i;
		}
		//all page are used.
		if(MemMap[i].freq < MemMap[min].freq)
			min = i;
	}
	assert(MemMap[min].lock == 0);
	page_out(min);
	return min;
}

/**
 * if the accessed bit is true, then set the frequent domain.
 */
void memmap_refresh(){
	int i;
	const byte_t used = 0x80;
	printf("memmap refreshing.\n");
	for(i = 0; i < MMAP_SIZE; i++){
		MemMap[i].freq >> 1;
		if(MemMap[i].accessed == TRUE){
			MemMap[i].freq |= used;
			MemMap[i].accessed = FALSE;
		}
	}
}
