/* Title: CIS 415 Project 1
 * Description: Part 2 - USPS takes control
 * Author: Melodie Collins
 * DuckID: mcolli11
 * This is my own work.*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "p1fxns.h"

int main(int argc, char *argv[])
{
	char *p;
	int val = -1;
	if((p = getenv("USPS_QUANTUM_MSEC")) != NULL){ // has a value
		val = atoi(p);
		if(val == 0) {
			printf("Environment variable USPS_QUANTUM_MSEC if specified must be an integer.\n");
			exit(EXIT_FAILURE);
		}
	}
	opterr = 0;
	int opt;
	while((opt = getopt(argc, argv, "q:")) != -1){
		switch(opt){
			case 'q': val = atoi(optarg); 
				  if(val == 0) {
					  printf("-q if specified must be an integer.\n");
					  exit(EXIT_FAILURE);
				  }
				  break;
		}
	}
	if(optind >= argc){
		printf("Missing file name.\n");
		exit(EXIT_FAILURE);
	}

	char *file = argv[optind];
	char buf[BUFSIZ];
	int c;
	int fd = open(file, O_RDONLY);
	int count = 0;

	c = p1getline(fd, buf, BUFSIZ); // this returns 0 if EOF
	
	while(c != 0){
		count++;
		c = p1getline(fd, buf, BUFSIZ);
	}
	close(fd);
	
	// malloc number of commands to char pointer array
	char **program = (char**)malloc(sizeof(char*) * count);
	// malloc number of commands with arguments to char pointer array
	char ***args = (char***)malloc(sizeof(char**) * count);

	fd = open(file, O_RDONLY);
	char word[BUFSIZ];
	c = p1getline(fd, buf, BUFSIZ); // this returns 0 if EOF
	int line = 0;
	char *tempArgs[100];
	
	while(c != 0){
		buf[p1strlen(buf) - 1] = '\0';

		int nextWord = p1getword(buf, 0, word); // current command
		if(nextWord == -1){
			// no command, EOF
			break;
		}
		program[line] = p1strdup(word);

		int argNum = 0;
		// loop through each item in the line
		while((nextWord = p1getword(buf, nextWord, word)) != -1){
			tempArgs[argNum] = p1strdup(word);
			argNum++;
		}
		
		char **lineArgs = (char**)malloc(sizeof(char*) * (argNum + 2));
		lineArgs[0] = p1strdup(program[line]);
		for(int i=1; i <= argNum; i++){
			lineArgs[i] = tempArgs[i-1];
		}
		lineArgs[argNum+1] = (char *)NULL;
		args[line] = lineArgs;
		c = p1getline(fd, buf, BUFSIZ);
		line++;
	}
	close(fd);

	int *pid = (int*)malloc(sizeof(int) * count);
	for(int i=0; i < count; i++){
		// fork creates copy of self and assigns it an ID
		pid_t p = fork();
		if(p<0){
			printf("Unable to fork.\n");
		}
		
		// child
		if(p == 0){
			int sig;
			sigset_t set;
			sigemptyset(&set);
			sigaddset(&set, SIGUSR1);
			sigaddset(&set, SIGCONT);
			sigprocmask(SIG_SETMASK, &set, NULL);

			// wait for signal
			sigwait(&set, &sig);
			// change current program with different program
			execvp(program[i], args[i]);
		}
		// parent
		else{
			pid[i] = p;
		}	
		
	}
	sleep(1);
	for(int i=0; i < count; i++){
	/* After all processes created and waiting for SIGUSR 
	 * signal, the USPS parent process sends each program
	 * SIGUSR signal to wake them up. Each child process
	 * will then invoke execvp().*/
		kill(pid[i], SIGUSR1);
	}

	for(int i=0; i < count; i++){
	/* Aftter all processes awakened and executing, USPS 
	 * sends each process SIGSTOP signal to suspend it.*/
		kill(pid[i], SIGSTOP);
	}

	for(int i=0; i < count; i++){
		/* After all workload processes have been suspended, 
		 * USPS sends each process SIGCONT signal to resume it.*/
		kill(pid[i], SIGCONT);
	}

	for(int i=0; i < count; i++){
		// Once all programs are running, wait for each process to terminate.
		wait(&pid[i]);
	}

	for(int i = 0; i < count; i++){
		free(program[i]);
		int index = 0;
		while(args[i][index] != NULL){
			free(args[i][index++]);
		}
		free(args[i]);
	}
	free(program);
	free(args);
	free(pid);
	return EXIT_SUCCESS;
}
