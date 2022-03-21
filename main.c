#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#define BUFSIZE 4096

const char *fn1 = "file1.txt";
const char *fn2 = "file2.txt";

void makeWork(const char *fn, char *buf, int out) {
	int fd = open(fn, O_RDONLY);
	int bytes;
	if (fd < 0) {
		fprintf(stderr, "Can't open file %s\n", fn);
		exit(0);
	}
	while((bytes = read(fd, buf, BUFSIZE)) > 0) write(out, buf, bytes);
	printf("Child process %c done\n", fn[4]);
	fflush(stdout);
}

int main() {
	int proc[2], err, p[2][2], nfds, bytes;
	struct timeval tv;
	fd_set rfds;
	char buf[BUFSIZE];
	srand(time(NULL));
	printf("Starting main process\n");
	
	//opening pipe
	if(pipe(p[0]) < 0) {
		fprintf(stderr, "pipe error\n");
		exit(1);
	}

	//make first fork
	proc[0] = fork();
	if(proc[0] < 0) {
		fprintf(stderr, "an error occured with fork\n");
		exit(1);
	}
	//first child process
	if(proc[0] == 0) {
		close(p[0][0]);
		makeWork(fn1, buf, p[0][1]);
		close(p[0][1]);
		close(p[1][0]);
		close(p[1][1]);
		exit(0);
	}

	//opening pipe
	if(pipe(p[1]) < 0) {
		fprintf(stderr, "pipe error\n");
		exit(1);
	}

	//make second fork
	proc[1] = fork();
	if(proc[1] < 0) {
		fprintf(stderr, "an error occured with fork\n");
		kill(proc[0], SIGKILL);		//kill first child
		exit(1);
	}
	//second child process
	if(proc[1] == 0) {
		close(p[1][0]);
		close(p[0][0]);
		close(p[0][1]);
		makeWork(fn2, buf, p[1][1]);
		close(p[1][1]);
		exit(0);
	}
	
	close(p[0][1]);
	close(p[1][1]);
	nfds = p[1][1] + 1;
	int pid[2] = {0,0};
	for(;pid[0] == 0 || pid[1] == 0;) {
		//add 0 and 1 file descriptors in set
		FD_ZERO(&rfds);
		FD_SET(p[0][0], &rfds);
		FD_SET(p[1][0], &rfds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		err = select(nfds, &rfds, NULL, NULL, &tv);
		if(err == 0) {			//timeout
			printf("Nothing happening, I'm shutdown...\n");
			wait(NULL);
			wait(NULL);
			exit(0);
		} else if (err == -1) {	//error
			fprintf(stderr, "Fail to select\n");
			kill(proc[0], SIGKILL);
			kill(proc[1], SIGKILL);
			exit(5);
		} else {				//success
			for(int i = 0; i < nfds; i++) {
				if(FD_ISSET(i, &rfds))
					while((bytes = read(i, buf, BUFSIZE)) > 0) {
						for(int j = 0; j < bytes; j++)
							buf[j] ^= (char)(rand() % 256);
						write(1, buf, bytes);
						fflush(stdout);
					}
			}
		}
	//	printf("i'am here\n");
		pid[0] = waitpid(proc[0], &err, WNOHANG);
		pid[1] = waitpid(proc[1], &err, WNOHANG);
	}
	exit(0);
}
