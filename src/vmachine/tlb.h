#ifndef TLB_H
#define TLB_H
extern tlbe_t TLB[TLB_SIZE];

void init_tlb();
int lookup_tlb(address_t);
void tlb2pgt(int);
void tlb_clear();
int tlb_swap(pte_t *, int);
void pgt2tle(int, pte_t *, int);

#endif
