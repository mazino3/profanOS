#include <driver/serial.h>
#include <kernel/task.h>
#include <minilib.h>
#include <system.h>


static task_t tasks[TASK_MAX_COUNT + 1];
int current_pid, task_count;

/***********************
 * INTERNAL FUNCTIONS *
***********************/

void i_new_task(task_t *task, void (*main)(), uint32_t flags, uint32_t *pagedir, int pid) {
    uint32_t esp_alloc = (uint32_t) malloc(TASK_ESP_ALLOC);
    task->regs.eax = 0;
    task->regs.ebx = 0;
    task->regs.ecx = 0;
    task->regs.edx = 0;
    task->regs.esi = 0;
    task->regs.edi = 0;
    task->regs.eflags = flags;
    task->regs.eip = (uint32_t) main;
    task->regs.cr3 = (uint32_t) pagedir;
    task->regs.esp = esp_alloc + TASK_ESP_ALLOC;
    task->esp_addr = esp_alloc;
    task->pid = pid;
    task->isdead = 0;
}

void i_destroy_killed_tasks(int nb_alive) {
    for (int i = 1; i < nb_alive; i++) {
        if (tasks[i].isdead != 1) continue;
        free((void *) tasks[i].esp_addr);
        tasks[i].isdead = 2;
    }
}

/***********************
 * EXTERNAL FUNCTIONS *
***********************/

void tasking_init() {
    static task_t mainTask;

    // Get EFLAGS and CR3
    asm volatile(
        "movl %%cr3, %%eax\n\t"
        "movl %%eax, %0"
        : "=m" (mainTask.regs.cr3)
        :: "%eax");

    asm volatile("pushfl\n\t"
        "movl (%%esp), %%eax\n\t"
        "movl %%eax, %0\n\t"
        "popfl"
        : "=m"(mainTask.regs.eflags)
        :: "%eax");

    str_cpy(mainTask.name, "kernel");
    mainTask.isdead = 0;
    mainTask.pid = 0;

    tasks[0] = mainTask;

    current_pid = 0;
    task_count = 1;
}

int task_create(void (*func)(), char *name) {
    int nb_alive = task_get_alive();
    if (task_count >= TASK_MAX_COUNT) {
        sys_fatal("Cannot create task, too many tasks");
        return -1;
    }
    current_pid++;
    int pid = current_pid;
    task_t task, *mainTask;
    for (int i = 0; i < nb_alive; i++) {
        if (tasks[i].pid == 0) {
            mainTask = &tasks[i];
            break;
        }
    }

    str_cpy(task.name, name);

    i_new_task(&task, func, mainTask->regs.eflags, (uint32_t*) mainTask->regs.cr3, pid);
    tasks[nb_alive] = task;
    task_count++;
    return pid;
}

void task_switch(int target_pid) {
    int task_i, nb_alive = task_get_alive();

    if (tasks[0].pid == target_pid) {
        sys_error("Cannot switch to self");
        return;
    }

    for (task_i = 0; task_i < nb_alive; task_i++) {
        if (tasks[task_i].pid == target_pid) {
            tasks[TASK_MAX_COUNT] = tasks[task_i];
            for (int i = task_i; i > 0; i--) {
                tasks[i] = tasks[i - 1];
            }
            tasks[0] = tasks[TASK_MAX_COUNT];
            break;
        } else if (task_i == nb_alive - 1) {
            sys_error("Task not found");
            return;
        }
    }

    task_asm_switch(&tasks[1].regs, &tasks[0].regs);
    i_destroy_killed_tasks(nb_alive);
}

void task_kill_task_switch(int target_pid) {
    tasks[0].isdead = 1;
    task_switch(target_pid);
}

void task_kill(int target_pid) {
    int nb_alive = task_get_alive();
    for (int i = 0; i < nb_alive; i++) {
        if (tasks[i].pid == target_pid) {
            tasks[i].isdead = 1;
            i_destroy_killed_tasks(nb_alive);
            return;
        }
    }
    sys_error("Task not found in kill");
}

/******************
 * GET FUNCTIONS *
******************/

int task_get_alive() {
    int decal = 0, nb_alive = 0;
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].isdead == 2) {
            decal++;
        } else {
            nb_alive++;
            if (decal > 0) tasks[i - decal] = tasks[i];
        }
    }
    task_count = nb_alive;
    return nb_alive;
}

int task_get_current_pid() {
    return tasks[0].pid;
}

int task_get_next_pid() {
    task_get_alive();
    return tasks[1].pid;
}

int task_get_max() {
    return TASK_MAX_COUNT;
}

int task_get_internal_pos(int pid) {
    int nb_alive = task_get_alive();
    for (int i = 0; i < nb_alive; i++) {
        if (tasks[i].pid == pid) {
            return i;
        }
    }
    sys_error("Task not found");
    return -1;
}

void task_set_bin_mem(int pid, uint8_t *bin_mem) {
    tasks[task_get_internal_pos(pid)].bin_mem = bin_mem;
}

uint8_t *task_get_bin_mem(int pid) {
    return tasks[task_get_internal_pos(pid)].bin_mem;
}

char *task_get_name(int internal_pos) {
    return tasks[internal_pos].name;
}

int task_get_pid(int internal_pos) {
    return tasks[internal_pos].pid;
}
