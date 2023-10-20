/*
* small program to count cycles of a command
*
* usage: [sudo] cycles [cmd]
* example: sudo cycles /usr/bin/find /etc
*
* kai_AT_doernemann.net 2023-10-15
*/

#define _GNU_SOURCE

#include <linux/hw_breakpoint.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define elemof(x)	((sizeof(x))/(sizeof(*x)))

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

// convert "1,2,3" to array[1,2,3]
int
arrayofuint(const char * const x, size_t maxidx, u_int *arr)
{
	int v;
	int idx = 0;
	char c;
	const char *s = x;

	while ((c = *s++) != 0) {
		if (isdigit(c)) {
			v = 0;
			while (isdigit(c)) {
				v *= 10;
				v += c - '0';
				c = *s++;
			}
			arr[idx++] = v;
			if (idx >= maxidx)
				return idx;
		}
		if (isalpha(c) || c == '_') {
			char grname[32];
			int n = 0;
			while ((n < sizeof(grname)-1) && (isalnum(c) || c == '_')) {
				grname[n++] = c;
				c = *s++;
			}
			grname[n++] = '\0';
			struct group *g = getgrnam(grname);
			if (g == NULL)
				err(1, "getgrnam(%s)", grname);
			arr[idx++] = g->gr_gid;
		}
		if (c == '\0')
			return idx;
		if (!(isspace(c) || c == ':' || c == ','))
			errx(1, "illegal character '%c' in numeric string '%s'", c, x);
	}
	return idx;
}

/* set uid / gids if supplied.
   if uid != 0 and gidcount == 0, the default gid of user uid is set
   if gidcount > 0, the gids in gids are set
   uid is only set if uid != 0
*/
void
setuidgid(uid_t uid, int gidcount, const gid_t * const gids)
{
	if (gidcount != 0) {
		/* someone supplied a list of gids with -g */
		if (setgroups(gidcount, gids) < 0)
			err(1, "setgroups");
		if (setresgid(gids[0], gids[0], gids[0]) < 0)
			err(1, "setresgid");
	} else {
		/* no gids on cmdline, use default gid of corresponding uid != 0 */
		if (uid != 0) {
			struct passwd *pw;
			if ((pw = getpwuid(uid)) == NULL)
				err(1, "getpwuid(%d)", uid);
			if (setgroups(0, NULL) < 0)
				err(1, "setgroups");
			if (setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid) < 0)
				err(1, "setresgid(%d)", pw->pw_gid);
		}
	}
	if (uid != 0)
		if (setresuid(uid, uid, uid) < 0)
			err(1, "setresuid");
}

int
main(int argc, char *argv[])
{
	pid_t pid;

	uid_t uid = 0;
	int gidcount = 0;
	gid_t gids[10];

	unsigned long x;
	char *endptr;

	struct passwd *pwent;

	int opt;
	while ((opt = getopt(argc, argv, "u:g:")) != -1) {
		switch (opt) {
			case 'u':
				/* set user id in child process */
				pwent = getpwnam(optarg);
				if (pwent != NULL) {
					uid = pwent->pw_uid;
				} else {
					x = strtoul(optarg, &endptr, 10);
					if (*endptr != '\0')
						errx(1, "parameter '%s' not numeric and not a groupname", optarg);
					uid = (uid_t) x;
				}
				break;
			case 'g':
				/* set groups list in child process */
				gidcount = arrayofuint(optarg, elemof(gids), gids);
				break;
			default:
				errx(1, "unknown option %c", (char) opt);
		}
	}

	if (optind >= argc)
		errx(1, "no command");

	int perf_fd = cycles_count_start();

	pid = fork();
	if (pid < 0)
		err(1, "fork");
	else if (pid == 0) {
		setuidgid(uid, gidcount, gids);

		/* reset counters */
		ioctl(perf_fd, PERF_EVENT_IOC_RESET, 0);
		ioctl(perf_fd, PERF_EVENT_IOC_ENABLE, 0);

		/* run command */
		execve(argv[optind], &argv[optind], NULL);
		err(1, "execve(%s)", argv[optind]);
	}

	int wstatus;
	if (wait(&wstatus) < 0)
		err(1, "wait");
	fprintf(stderr, "cycles: %lld\n", cycles_count_stop(perf_fd));
	exit(WEXITSTATUS(wstatus));
}