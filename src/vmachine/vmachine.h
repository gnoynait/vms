#ifndef VMACHINE_H
#define VMACHINE_H
extern byte_t Memory[MEM_SIZE];
extern address_t Address;
extern mode_t AddrMode;
void mmap_refresh_handler(int);
void start_machine();
void shutdown_machine();
void do_reponse(request_t *);
bool access_addr(address_t, command_t);
void vm_fork(pid_t, task_t*);
pgd_t *pgd_deep_clone(task_t *);
void exec_program(task_t *, program_t *);
//void load_seg(pgd_t*, seg_t*, mod_t);
//void load_program(task_t*, program_t*);
void load_program(task_t *, program_t *);
void lazy_load(task_t *task, int page);
pte_t * new_pgtable(mod_t mode);
void reset_pte(const pte_t *);
void free_pgd(const pgd_t);
#endif
