#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef int tarox_task_t;
typedef int (*tarox_main_t)(int argc, char *argv[]);

tarox_task_t tarox_task_spawn_cmd(const char *name, int priority, int stack_size,
                                  tarox_main_t entry, char *const argv[]);
int tarox_task_delete(tarox_task_t task_id);
tarox_task_t tarox_task_id(void);
void tarox_task_exit(int status);

#ifdef __cplusplus
}
#endif

