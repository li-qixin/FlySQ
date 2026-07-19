#include <tarox/tasks.h>

#include <sched.h>
#include <unistd.h>

extern "C"
{

tarox_task_t tarox_task_spawn_cmd(const char *name, int priority, int stack_size,
                                  tarox_main_t entry, char *const argv[])
{
	return task_create(name, priority, stack_size, entry, argv);
}

int tarox_task_delete(tarox_task_t task_id)
{
	return task_delete(task_id);
}

tarox_task_t tarox_task_id(void)
{
	return getpid();
}

void tarox_task_exit(int status)
{
	_exit(status);
}

}
