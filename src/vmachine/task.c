#include "vmm.h"
#include "task.h"
#include "program.h"
#include "communicate.h"

task_t TaskInit;
task_t * CurTask;/* current task */

void init_task()
{
	int i;
	TaskInit.pid = 0;
	TaskInit.pgd = calloc(PGD_N, sizeof (pgd_t));
	TaskInit.next = NULL;
	CurTask = &TaskInit;
	//	CurTask->pgd = calloc (PGD_N, sizeof(pgd_t));
	load_program(&TaskInit, &Program[0]);
}

/**
 * generate a new process
 */
task_t *new_task(pid_t pid, pgd_t *pgd)
{
	task_t *new_t;
	char fifo_name[50];
	int i;
	printf("new task %d.\n", pid);
	if((new_t = (task_t *) malloc(sizeof(task_t))) == NULL)
		return NULL;
	new_t->pid = pid;
	new_t->pgd = pgd;
	new_t->next = TaskInit.next;
	sprintf(fifo_name, CLIENT_FIFO_FORM, pid);
	new_t->fifo_fd = open(fifo_name, O_WRONLY);
	TaskInit.next = new_t;
	return new_t;
}

/**
 * switch to task pid.
 * return FALSE if failed.
 */
bool switch_task(pid_t pid)
{
	task_t *p = &TaskInit;
	printf("switching task to %d.\n", pid);
	tlb_clear();
	while(p){
		if(p->pid == pid){
			CurTask = p;
			return TRUE;
		}
		p = p->next;
	}
	return FALSE;
}

/**
 * free a task's resources, and return its next task.
 */
task_t *free_task(task_t *task)
{
	task_t *next = task->next;
	int i, j;
	close (task->fifo_fd);
	for(i = 0; i < PGD_N; i++){
		if(task->pgd[i] == NULL)
			continue;
		for(j = 0; j < PTE_N; j++){
			reset_pte(&(task->pgd[i][j]));
		}
		free(task->pgd[i]);
	}
	return next;
}

/**
 * kill a task ,free it's memory, fix page table,quick table.
 * if pid = 0, kill all task;
 */
void kill_task(pid_t pid)
{
	task_t *p = TaskInit.next;
	task_t *t = &TaskInit;
	while(p){
		if (p->pid == pid || pid == 0) {
			t->next = free_task(p);
			return;
		}
		t = p;
		p = p->next;
	}
}
