#include "vmm.h"
#include "dev.h"
#include "task.h"
#include "vmachine.h"
//translation lookup table
tlbe_t TLB[TLB_SIZE];

void init_tlb(){
    int i;
    for(i = 0; i < TLB_SIZE; i++){
	TLB[i].valid = FALSE;
	TLB[i].freq = 0x00;
    }
}

/**
 * look the page_num from the TLB
 * pg_n: the virtual page number;
 * type: read or write
 * if succesus return the page_num
 * if the tpye is write but the page is note writable, report a error.
 * else return -1
 */
int lookup_tlb(address_t vaddress){
    int i;
    for(i = 0; i < TLB_SIZE; i++){
	// refresh the frequence, for kick out.
	TLB[i].freq >>= 1;
	if(TLB[i].valid && TLB[i].vpage == A2PAGE(vaddress)){
	    TLB[i].freq |= 0x80;
	    AddrMode = TLB[i].mode;
	    Address = (TLB[i].ppage << OFFSET_BIT) | A2OFFSET(vaddress);
	    return i;
	    printf("lookup TLB: found %d\n", i);
	}
    }
    printf("lookup TLB: not found.\n");
    return -1;
}

/**
 * put the TLB[out] to the pages if neccessary.
 */
void tlb2pgt(out){
    int page = TLB[out].ppage;
    pte_t *pte;
    pte = &CurTask->pgd[P2PGD(page)][P2PTE(page)];
    if(TLB[out].valid == TRUE ){
	    if( TLB[out].edited == TRUE)
		    pte->edited = TRUE;
	pte->page_ID = TLB[out].ppage; /* changed when copy */
    }
    TLB[out].valid = FALSE;
    TLB[out].freq = 0x00;
}

/**
 * put a pte in the page table in to the TLB[in].
 * vpage: virtual page number.
 */
void pgt2tlb(int vpage, pte_t* pte, int in){
    TLB[in].ppage = pte->page_ID;
    TLB[in].vpage = vpage;
    TLB[in].edited = FALSE;
    TLB[in].mode = pte->mode;
    TLB[in].valid = TRUE;
    TLB[in].freq = 0x00;
}

/**
 * set all the TLB item unvalible. used when task 
 * switches. update the page if neccessory.
 */
void tlb_clear(){
    int i;
    printf("TLB clear.\n");
    for(i = 0; i < TLB_SIZE; i++){
	tlb2pgt(i);
	TLB[i].valid = FALSE;
    }
}

/**
 * Swap the TLB for a new item
 * pte: pte to swap in.
 * vpage: virtual page.
 * if there is a not used item in the TLB, use it.else use the least frenquent 
 * one. if it's is dirty, update the previous pge.
 */
int tlb_swap(pte_t *pte, int vpage)
{
    int i;
    int out = 0;
    int min = TLB[0].freq;
    printf("TLB swapping\n");
    for(i = 0; i < TLB_SIZE; i ++){
	if(TLB[i].valid == FALSE){//not used yet.
	    out = i;
	    break;
	}
	if(TLB[i].freq < min){// recently less used.
	    out = i;
	    min = TLB[i].freq;
	}
    }
    tlb2pgt(out);
    pgt2tlb(vpage, pte, out);
    return out;
}
