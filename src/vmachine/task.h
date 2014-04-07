#ifndef TASK_H
#define TASK_H
extern task_t TaskInit;
extern task_t *CurTask;

void init_task();
task_t *new_task(pid_t, pgd_t*);
bool switch_task(pid_t);
task_t *free_task(task_t *);
void kill_task(pid_t pid);

#endif
