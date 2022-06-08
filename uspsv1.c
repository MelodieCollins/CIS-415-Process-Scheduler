/* Title: CIS 415 Project 1
 * Description: Part 1 - USPS launches the workload
 * Author: Melodie Collins
 * DuckID: mcolli11
 * This is my own work.*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "p1fxns.h"

/*void printArrays(char* program[], char** args[], int lines){
	for(int i=0; i < lines; i++){
		printf("program %d: %s\n", i, program[i]);
		int ind = 0;
		while(args[i][ind] != NULL){
			printf("args %d %d: %s\n", i, ind, args[i][ind]);
			ind++;
		}
		printf("args %d %d: %s\n", i, ind, args[i][ind]);
	}
}*/


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

	//printArrays(program, args, line);

	int *pid = (int*)malloc(sizeof(int) * count);
	for(int i=0; i < count; i++){
		// fork creates copy of self and assigns it an ID
		pid[i] = fork();
		if(pid[i]<0){
			printf("Fork failed.\n");
			exit(1);
		}
		if(pid[i] == 0){
			// change current program with different program
			execvp(program[i], args[i]);
		}
		for(int i=0; i < count; i++){
			// Once all programs are running, wait for each process to terminate.
			wait(&pid[i]);
		}
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
