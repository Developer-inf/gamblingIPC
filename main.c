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

int main() {
	//precesses, error, pipes,	num of file descriptors, bytes
	int proc[2], err,	p[2][2],nfds,					 bytes;
	int pid[2] = {0,0};	//pid of child process
	struct timeval tv;	//time structure
	fd_set rfds;		//read file descriptor
	char buf[BUFSIZE];	//buffer
	srand(time(NULL));	//set seed generator

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
		dup2(p[0][1], 1);
		execlp("cat", "cat", "file1.txt", NULL);
		perror("Something goes wrong with 1st child\n");
		exit(1);
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
		dup2(p[1][1], 1);
		execlp("ls", "ls", "-la", NULL);
		perror("Something goes wrong with 2nd child\n");
		exit(2);
	}
	
	close(p[0][1]);
	close(p[1][1]);
	nfds = p[1][1] + 1;
	for(;pid[0] == 0 || pid[1] == 0;) {
		//add 0 and 1 file descriptors in set
		FD_ZERO(&rfds);			//clear set of read file descriptors
		FD_SET(p[0][0], &rfds);	//set read of 1st pipe to rfds
		FD_SET(p[1][0], &rfds);	//set read of 1st pipe to rfds
		tv.tv_sec = 0;			//seconds of waiting select
		tv.tv_usec = 500000;	//microseconds

		err = select(nfds, &rfds, NULL, NULL, &tv);	//select ready descs
		if(err == 0) {			//timeout
			printf("Nothing happening, I'm shutdown...\n");
			wait(NULL);
			wait(NULL);
			exit(0);
		} else if (err < 0) {	//error
			fprintf(stderr, "Fail to select\n");
			kill(proc[0], SIGKILL);
			kill(proc[1], SIGKILL);
			exit(5);
		} else {				//success
			for(int i = 0; i < nfds; i++) {
				if(FD_ISSET(i, &rfds)) {	//if rfd[i] is ready to read
					while((bytes = read(i, buf, BUFSIZE)) > 0) {
						printf("\nStart writing\n");
						for(int j = 0; j < bytes; j++)
							buf[j] ^= (char)(rand() % 256 & 255);	//XOR
						write(1, buf, bytes);
						printf("\nEnd writing\n");
						fflush(stdout);
					}
				}
			}
		}
		//seek if child precesses done
		pid[0] = waitpid(proc[0], &err, WNOHANG);
		pid[1] = waitpid(proc[1], &err, WNOHANG);
	}
	exit(0);
}
