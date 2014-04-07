#ifndef VMM_H
#define VMM_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<assert.h>
typedef unsigned short address_t;
typedef unsigned char  byte_t;

typedef enum{
	TRUE = 1,
	FALSE = 0,
}bool;

#define PGD_BIT 4
#define PTE_BIT 4
#define OFFSET_BIT 8

#define PGD_N 16
#define PTE_N 16

#define PROG_N 10
#define SEG_N 10

#define MMAP_SIZE 32
#define PAGE_SIZE (1u << OFFSET_BIT)
#define MEM_SIZE (MMAP_SIZE * PAGE_SIZE)

#define TLB_SIZE 8

#define SWAP_FNAME "swap"
#define FS_FNAME "FSfile"
#define SWAP_SIZE 256
#define FS_SIZE 256

#define MMAP_REFRESH_TIME 10

//macros for address translation.

#define PTE(pgdn, pgn) (CurTask->pgd[(pgdn)][(pgn)])
//page to pte and pgd
#define P2PTE(page) ((page) & 0x000F)
#define P2PGD(page) (((page) >> PTE_BIT) & 0x000F)
//address to pte and pgd
#define A2PTE(addr) (((addr) >> OFFSET_BIT) & 0x000F)
#define A2PGD(addr) (((addr) >> (OFFSET_BIT + PTE_BIT) ) & 0x000F)
#define A2PAGE(addr) ((addr) >> OFFSET_BIT)
#define A2OFFSET(addr) ((addr) & 0x00FF)
#define PAGE(pgd, pte) (((pgd) << PTE_BIT) | (pte))
#define ADDR(pgd, pte, offset) (((pgd) << (PTE_BIT + OFFSET_BIT)) |	\
				((pte) << OFFSET_BIT) | (offset))

/* executable, readable, writable, copy */
typedef unsigned char mod_t;
#define MODE_RWX 0x07
#define MODE_RW 0x06
#define MODE_W 0x02
#define MODE_R 0x04
#define MODE_X 0x01
#define MODE_C 0x08


typedef struct {
	bool valid;//in memory?
	bool swapped;// TRUE if in swap; FALSE if in FS
	bool edited;
	
	mod_t mode;
	//	bool copy; // write on copy

	int page_ID;
}pte_t;

typedef struct{
	int start_block;// the first block in the disk.
	int data; //the length of data area, whitch is readonly
	int text;//length of text area, whitch is readonly and excutable
}program_t;

typedef struct{
	int lock;// equals to refer number minus 1
	bool accessed;
	byte_t freq;
	pte_t *ref_pte;
	int block; /* block in the  */
}page_t;

typedef pte_t* pgd_t;

/* TLB enty type */
typedef struct{
	int vpage; // virtual page
	int ppage;// physic page
	bool edited;
	mod_t mode;
	bool valid;//True if it's used.
	int freq;
}tlbe_t;

typedef struct _task_t{
	pid_t pid;
	pgd_t *pgd;
	program_t *program;
	struct _task_t * next;
	int fifo_fd;
}task_t;

typedef enum{
	NEW_TASK,
	READ,
	WRITE,
	EXEC,
	RUN,
	DISPLAY,
	QUIT,
	SHUT_DOWN,
}command_t;

typedef struct{
	pid_t pid;
	command_t command;
	address_t address;
	byte_t value;
}request_t;

typedef struct _block_t{
	int page_ID;//if loaded, the phisic page
	int count; // reference count
}fs_block_t;

#endif
