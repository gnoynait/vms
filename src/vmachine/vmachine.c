#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include"vmm.h"
#include"vmachine.h"
#include"dev.h"
#include"task.h"
#include"memmap.h"
#include"program.h"
#include"tlb.h"
#include"communicate.h"

//#define SINGLE_DEBUG


byte_t Memory[MEM_SIZE];
address_t Address;
mode_t AddrMode;

char Buffer[BUFF_SIZE];

/**
 * To handler the SIGALRM. refresh the memmap accessed domain.
 */
void mmap_refresh_handler(int sig)
{
	if(sig == SIGALRM){
		memmap_refresh();
		alarm(MMAP_REFRESH_TIME);
	}
}

/**
 * Do some initial jobs.
 */
void start_machine()
{
	init_tlb();
	mount_dev();
	init_memmap();
	init_task();
	signal(SIGALRM, mmap_refresh_handler);
	alarm(1);
}

/**
 * free resources.
 */
void shutdown_machine()
{
	//kill all tasks
	kill_task(0);
	unmount_dev();
}

/**
 * display info to the client and machine.
 */
static void display(char *info)
{
	if (info != Buffer)
		strcpy(Buffer, info);
#ifndef SINGLE_DEBUG
	write(CurTask->fifo_fd, Buffer, BUFF_SIZE);
#endif
	fprintf(stdout, "%s\n", info);
}

void display_pagetable(task_t *task){
	int i, j;
	pte_t *pte;
	sprintf(Buffer, "task %d page table\n", task->pid);
	display(Buffer);
	for (i = 0; i < PGD_N; i++) {
		if (task->pgd[i] == NULL)
			continue;
		for(j = 0; j < PTE_N; j++) {
			pte = &task->pgd[i][j];
			if (pte->page_ID == -1)
				continue;
			printf("vpage: %2x    ppage: %2x    valid: %d "
				"   swaped: %d    mode: %d    edited: %d\n",
				i * PGD_N + j, pte->page_ID, pte->valid,
				pte->swapped, pte->mode, pte->edited);
		}
	}
}

/**
 * Handle the client process's request.
 */
void do_response(request_t * request)
{
	pid_t pid = request->pid;
	if(request->command == NEW_TASK){
		new_task(pid, pgd_deep_clone(&TaskInit));
		switch_task(pid);
		display("Started\n");
		return;
	}
	if(pid != CurTask->pid)
		switch_task(pid);

	switch(request->command){
	case READ:
		if (access_addr(request->address,READ)){
			sprintf(Buffer, "read %x: %x\n",
				Address,
				Memory[Address]);
			display (Buffer);
		}
		break;
	case WRITE:
		if (access_addr(request->address, WRITE)){
			Memory[Address] = request->value;
			sprintf(Buffer, "write %x: %x\n",
				Address,
				Memory[Address]);
			display (Buffer);
		}
		break;
	case EXEC:
		if (access_addr(request->address,EXEC)){
			sprintf(Buffer, "exec %x\n",
				Address,
				Memory[Address]);
			display (Buffer);
		}
		break;
	case RUN:
		exec_program(CurTask, &Program[request->value]);
		display("run\n");
		break;
	case DISPLAY:
		display_pagetable(CurTask);
		break;
	case QUIT:
		kill_task(CurTask->pid);
		switch_task(TaskInit.pid);
		display("quited.\n");
		break;
	case SHUT_DOWN:
		kill_task(0);
		display("shut down.\n");
		exit(0);
		break;
	default:
		printf("undefined command.\n");
	}
}

/**
 * Copy a page from the memory to  another place in the memory.
 */
void inline memcopy(int dest, int src)
{
	memcpy(&Memory[dest * PAGE_SIZE], &Memory[src * PAGE_SIZE], PAGE_SIZE);
}

/**
 * Lookup the virtual address in the page table and put the phisic
 * address and it's mode to the global variable Address and AddrMode.
 */
pte_t *lookup_pgt(address_t vaddr)
{
	int page = A2PAGE(vaddr);
	int pgdn = P2PGD(page);
	int pten = P2PTE(page);
	pte_t *pte;

	if(CurTask->pgd[pgdn] == NULL)
		CurTask->pgd[pgdn] = new_pgtable(MODE_RW);
	
	pte = &CurTask->pgd[pgdn][pten];
	if(pte->valid == FALSE){
		page_in(pte);
	}
	fprintf(stdout, "lookup_pgt: vpage:%x, ppage: %x.\n", 
		A2PAGE(vaddr), pte->page_ID);
	Address = (pte->page_ID << OFFSET_BIT) | A2OFFSET(vaddr);
	AddrMode = pte->mode;
	return pte;
}

/**
 * read, write or execute on the virtual memory.
 * return TRUE if access ok. FALSE if somthing wrong.
 */
bool access_addr(address_t vaddr, command_t cmd)
{
	int ppage, vpage, pten, pgdn;
	pte_t *pte;
	int tlb_i = lookup_tlb(vaddr);
	if (tlb_i == -1) { // not found in TLB
		pte = lookup_pgt(vaddr);
		tlb_i = tlb_swap(pte, A2PAGE(vaddr));
	}
	if(cmd == WRITE){
		if(!(AddrMode & MODE_W)){
			display("PERMISSION DENY: can't write.\n");
			//kill_task(CurTask->pid);
			return FALSE;
		}
		if(AddrMode & MODE_C){
			int np;
			if(MemMap[A2PAGE(Address)].lock > 0) {
				np = empty_page();
				memcopy(np, A2PAGE(Address));
				assert(pte != NULL);
				MemMap[np].ref_pte = pte;
				reset_pte(pte);
				pte->page_ID = np;
				Address = (np << OFFSET_BIT) | A2OFFSET(vaddr);
			}
			AddrMode &= (~MODE_C);
			TLB[tlb_i].mode &= (~MODE_C);
		} else {
			TLB[tlb_i].edited = TRUE;
		}
	}
	else if(cmd == READ){
		if(!(AddrMode & MODE_R)){
			display ("PERMISSION DENY: can't read.\n");
			//kill_task(CurTask->pid);
			return FALSE;
		}
	}
	else if(cmd == EXEC){
		if(!(AddrMode & MODE_X)){
			display ("PERMISSION DENY: can't execute.\n");
			//kill_task(CurTask->pid);
			return FALSE;
		}
	}
	else{
		fprintf(stdout, "undefined command.\n");
		return FALSE;
	}
	MemMap[A2PAGE(Address)].accessed = TRUE;
	return TRUE;
}

/**
 * it's used as the vmachine's system call.
 * it will clone the task but copy on write.
 */
void vm_fork(pid_t child_id, task_t *ptask)
{
	printf("vm forking...\n");
	pgd_t *pgd = pgd_deep_clone(ptask);
	new_task(child_id, pgd);
}

/**
 * copy the entire task_pgd and its ptes.
 * modify both new and old pte cw to True if
 * it's writable.
 */
pgd_t * pgd_deep_clone(task_t *task)
{
	int i, j;
	pgd_t *new_pgd, *ppgd = task->pgd;
	new_pgd = calloc(PGD_N, sizeof(pgd_t));
	printf("clone page tables\n");
	for(i = 0; i < PGD_N; i++){
		if(ppgd[i] == NULL)
			continue;
		new_pgd[i] = calloc(PTE_N, sizeof(pte_t));

		for(j = 0; j < PTE_N; j++){
			if (ppgd[i][j].valid == FALSE
			    && ppgd[i][j].swapped == FALSE) {
				if (ppgd[i][j].page_ID >= 0
				    && ppgd[i][j].valid == TRUE) {
					FSBlock[ppgd[i][j].page_ID].count++;
					FSBlock[ppgd[i][j].page_ID].page_ID = 
						ppgd[i][j].page_ID;
				}
				memcpy(&new_pgd[i][j], &ppgd[i][j],
				       sizeof(pte_t));
			} else {
				if (ppgd[i][j].swapped == TRUE)
					page_in (&ppgd[i][j]);
				ppgd[i][j].mode |= MODE_C;
				memcpy(&new_pgd[i][j], &ppgd[i][j],
				       sizeof(pte_t));
				MemMap[ppgd[i][j].page_ID].lock++;
			}
				       
		}
	}
	return new_pgd;
}

/**
 * this is like the exec system call. 
 * it load the program from fs to replace the orignal 
 * program.
 */
void exec_program(task_t * task, program_t *pro)
{
	pgd_t *pgd = task->pgd;
	int i;
	tlb_clear();
	for(i = 0; i < PGD_N; i++){
		if(pgd[i]) {
			free_pgd(pgd[i]);
			pgd[i] = NULL;
		}
	}
	load_program(task, pro);
}

void lazy_load(task_t *task,  int page)
{
	fprintf(stdout, "lazy_load: page:%x.\n", page);
	program_t *program = task->program;
	pte_t *pte;
	if (task->pgd == NULL) 
		task->pgd = calloc (PGD_N, sizeof(pgd_t));
	if (task->pgd[P2PGD(page)] == NULL)
		task->pgd[P2PGD(page)] = new_pgtable(MODE_RW);
	pte = &task->pgd[P2PGD(page)][P2PTE(page)];
	if (page < program->data) {
		pte->mode = MODE_R;
		pte->page_ID = program->start_block + page;
	} else if (page < program->text + program->data) {
		pte->mode = MODE_R | MODE_X;
		pte->page_ID = program->start_block + page;
	} else {
		pte->mode = MODE_RW;
		pte->page_ID = -1;
	}
	pte->valid = FALSE;
	pte->edited = FALSE;
	pte->mode &= (~MODE_C);
	pte->swapped = FALSE;
}

/**
 * load a program to the task.
 */
void load_program(task_t * task, program_t *pro)
{
	int page;
	int size = pro->data + pro->text;
	task->program = pro;
	printf("loading program.\n");
	for (page = 0; page < size; page++)
		lazy_load(task, page);
}

/**
 * return a initialized new page table.
 */
pte_t * new_pgtable(mod_t mode)
{
	pte_t * pgtable;
	int i;
	printf("new page table.\n");
	pgtable = calloc(PTE_N, sizeof(pte_t));
	for(i = 0; i < PTE_N; i++){
		pgtable[i].valid = FALSE;
		pgtable[i].swapped = FALSE;
		pgtable[i].edited = FALSE;
		pgtable[i].mode = mode;
		pgtable[i].page_ID = -1;

	}
	return pgtable;
}

void reset_pte(const pte_t *pte)
{
	if(pte == NULL)
		return;
	if(pte->valid == FALSE && pte->swapped == TRUE)
		SwapCount[pte->page_ID] --;
	else if(pte->valid == TRUE)
		MemMap[pte->page_ID].lock--;
	else
		FSBlock[pte->page_ID].count--;
}

/**
 * Free the PGD's resources.
 */
void free_pgd(pgd_t pgd)
{
	int i;
	if(pgd == NULL)
		return;
	for(i = 0; i < PTE_N; i++)
		reset_pte(&pgd[i]);
	//WHY CAN'T FREE PGD????????
	free(pgd);
}

/**
 * For test the vmm without a tty.
 */
void test ()
{
	request_t reqs[30] = {
		{345, NEW_TASK,  0x0000, 0x42},
		{345, READ,      0x0053, 0x00},
		{345, WRITE,     0x0053, 0x43},
		{345, EXEC,      0x0043, 0x00},
		{345, RUN,       0x0000, 0x02},
		{345, QUIT,      0x0000, 0x00},
	};
	display_pagetable(CurTask);
	do_response(reqs + 0);
	display_memmap();
	display_pagetable(CurTask);
	do_response(reqs + 1);
	display_memmap();
	display_pagetable(CurTask);
	/*	do_response(reqs + 2);
	display_memmap();
	do_response(reqs + 3);
	display_memmap();
	do_response(reqs + 4);
	display_memmap();
	do_response(reqs + 5);
	display_memmap();*/
}

int main()
{
	int server_fifo;
	int readn;
	request_t request_buffer;
	start_machine();

#ifdef SINGLE_DEBUG
	display_memmap();
	display_pagetable(CurTask);
	test();
#endif
#ifndef SINGLE_DEBUG
	mkfifo(SERVER_FIFO, 0666);
	server_fifo = open(SERVER_FIFO, O_RDONLY);
	readn = read(server_fifo, &request_buffer, sizeof(request_t));
	while(1){
		if (readn > 0)
			do_response(&request_buffer);
		readn = read(server_fifo, &request_buffer, sizeof(request_t));
	}
#endif
}
