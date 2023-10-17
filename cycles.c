/*
* small program to count cycles of a command
*
* usage: [sudo] cycles [cmd]
* example: sudo cycles /usr/bin/find /etc
*
* kai_AT_doernemann.net 2023-10-15
*/

#include <linux/hw_breakpoint.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// man 2 perf_event_open
static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
		int cpu, int group_fd, unsigned long flags)
{
	return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

int
cycles_count_start(void)
{
	struct perf_event_attr pe;
	int perf_fd;

	memset(&pe, 0, sizeof(struct perf_event_attr));
	pe.size = sizeof(struct perf_event_attr);
	pe.type = PERF_TYPE_HARDWARE;
	pe.config = PERF_COUNT_HW_INSTRUCTIONS;
	pe.disabled = 1;
	pe.inherit = 1;
	pe.exclude_kernel = 1;
	pe.exclude_hv = 1;	// Don't count hypervisor events.
	pe.exclude_idle = 1;
	pe.enable_on_exec = 1;	// start counting for child only

	if ((perf_fd = perf_event_open(&pe, 0, -1, -1, 0)) < 0)
		err(1, "Error opening perf_fd (you must be root)");
	/* reset counters */
	ioctl(perf_fd, PERF_EVENT_IOC_RESET, 0);
	ioctl(perf_fd, PERF_EVENT_IOC_ENABLE, 0);

	return perf_fd;
}

long long
cycles_count_stop(int perf_fd)
{
	long long count;
	ioctl(perf_fd, PERF_EVENT_IOC_DISABLE, 0);
	read(perf_fd, &count, sizeof(long long));
	close(perf_fd);
	return count;
}

int
main(int argc, char *argv[])
{
	pid_t pid;

	if (argc < 2)
		errx(1, "no command");

	int perf_fd = cycles_count_start();

	pid = fork();
	if (pid < 0)
		err(1, "fork");
	else if (pid == 0) {
		/* run command */
		execve(argv[1], &argv[1], NULL);
		err(1, "execve(%s)", argv[1]);
	}

	int wstatus;
	if (wait(&wstatus) < 0)
		err(1, "wait");
	fprintf(stderr, "cycles: %lld\n", cycles_count_stop(perf_fd));
	exit(WEXITSTATUS(wstatus));
}