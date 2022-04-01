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

void decrypt() {
	char fName[256];
	char buf[BUFSIZE], keyBuf[BUFSIZE];
	int rfd, key;
	int bytes;
	printf("Enter filename to decrypt: ");
	scanf("%255s", fName);
	if ((rfd = open(fName, O_RDONLY)) < 0) {
		fprintf(stderr, "Can't open file %s\n", fName);
		exit(1);
	}
	printf("Enter key filename: ");
	scanf("%255s", fName);
	if ((key = open(fName, O_RDONLY)) < 0) {
		fprintf(stderr, "Can't open file %s\n", fName);
		exit(1);
	}
	printf("\n");
	while((bytes = read(rfd, buf, BUFSIZE)) > 0) {
		read(key, keyBuf, bytes);
		for(int i = 0; i < bytes; i++) {
			buf[i] ^= keyBuf[i];
		}
		write(1, buf, bytes);
	}
	exit(0);
}

int main() {
	//precesses, pipes,	  bytes
	int proc[2], p[2][2], bytes;
	char buf[BUFSIZE];	//buffer
	srand(time(NULL));	//set seed generator

	printf("If you want to decrypt file enter 'd': ");
	scanf("%s", buf);
	if(!strcmp(buf, "d"))
		decrypt();

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
//		close(p[0][1]);
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
//		close(p[1][1]);
		execlp("ls", "ls", "-la", NULL);
		perror("Something goes wrong with 2nd child\n");
		exit(2);
	}
	
	close(p[0][1]);
	close(p[1][1]);

	//first child
	char *name = "res1.txt";
	char *nameKey = "key1.txt";
	int outputFile, keyFile;
	if((outputFile = open(name, O_WRONLY|O_TRUNC|O_CREAT, 0666)) < 0) {
		fprintf(stderr, "can't open file %s\n", name);
		return 1;
	}
	if((keyFile = open(nameKey, O_WRONLY|O_TRUNC|O_CREAT, 0666)) < 0 ) {
		fprintf(stderr, "can't open file %s\n", nameKey);
		return 2;
	}
	while((bytes = read(p[0][0], buf, BUFSIZE)) > 0) {
		printf("\nStart writing 1\n");
		for(int j = 0; j < bytes; j++) {
			char randChar = (char)(rand() % 256 & 255);	//XOR
			buf[j] ^= randChar;
			write(keyFile, &randChar, 1);
		}
		write(1, buf, bytes);
		write(outputFile, buf, bytes);
		printf("\n%d bytes writed\n", bytes);
		printf("End writing\n");
		fflush(stdout);
	}
	if(close(outputFile)) fprintf(stderr, "Can't close file %s\n", name);
	if(close(keyFile)) fprintf(stderr, "Can't close file %s\n", name);

	//second child
	name = "res2.txt";
	nameKey = "key2.txt";
	if((outputFile = open(name, O_WRONLY|O_TRUNC|O_CREAT, 0666)) < 0) {
		fprintf(stderr, "can't open file %s\n", name);
		return 1;
	}
	if((keyFile = open(nameKey, O_WRONLY|O_TRUNC|O_CREAT, 0666)) < 0 ) {
		fprintf(stderr, "can't open file %s\n", nameKey);
		return 2;
	}
	while((bytes = read(p[1][0], buf, BUFSIZE)) > 0) {
		printf("\nStart writing 2\n");
		for(int j = 0; j < bytes; j++) {
			char randChar = (char)(rand() % 256 & 255);	//XOR
			buf[j] ^= randChar;
			write(keyFile, &randChar, 1);
		}
		write(1, buf, bytes);
		write(outputFile, buf, bytes);
		printf("\n%d bytes writed\n", bytes);
		printf("End writing\n");
		fflush(stdout);
	}
	if(close(outputFile)) fprintf(stderr, "Can't close file %s\n", name);
	if(close(keyFile)) fprintf(stderr, "Can't close file %s\n", name);
	exit(0);
}
