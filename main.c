#include <global.h>

int main(int argc __attribute__((__unused__)), char *argv[]){
	struct rlimit limits;
	getrlimit(RLIMIT_AS, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_AS, &limits);
	getrlimit(RLIMIT_CORE, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_CORE, &limits);
	getrlimit(RLIMIT_CPU, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_CPU, &limits);
	getrlimit(RLIMIT_DATA, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_DATA, &limits);
	getrlimit(RLIMIT_FSIZE, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_FSIZE, &limits);
	getrlimit(RLIMIT_NOFILE, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_NOFILE, &limits);
	getrlimit(RLIMIT_NPROC, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_NPROC, &limits);
	getrlimit(RLIMIT_SIGPENDING, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_SIGPENDING, &limits);
	getrlimit(RLIMIT_STACK, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_STACK, &limits);
	return 0;
}
