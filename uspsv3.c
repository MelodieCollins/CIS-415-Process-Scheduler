/* Title: CIS 415 Project 1
 * Description: Part 3 - USPS takes control
 * Author: Melodie Collins
 * DuckID: mcolli11
 * This is my own work.*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "p1fxns.h"
#include "ADTs/queue.h"

#define UNUSED __attribute__((unused))
#define MAX 128
#define MAXBUF 4096
#define MIN_QUANTUM 20
#define MAX_QUANTUM 2000
#define MS_PER_TICK 20

typedef struct pcb {
	pid_t pid;
	int ticks;
	bool isalive;
	bool usr1;
} PCB;

PCB array[MAX];
volatile int active_procs = 0;
int num_procs = 0;
volatile int usr1 = 0;
pid_t parent;
const Queue *q = NULL;
PCB *current = NULL;
int quantum_ticks = 0;

static int pid2index(pid_t pid){
	int i;
	for(i = 0; i < num_procs; ++i){
		if(array[i].pid == pid)
			return i;
	}
	return -1;
}

static void usr1_handler(UNUSED int sig){
	usr1++;
}

static void usr2_handler(UNUSED int sig){
}

static void chld_handler(UNUSED int sig){
	//PID
	//pid = wait(-1, status, WNOHANG)
	//satus of the process is actually dead
	//	active_procs--;
	//	array[pid].isalive = false;
	//	signal(parent, USR2)
	
	pid_t pid;
	int status;
	while((pid = waitpid(-1, &status, WNOHANG)) > 0){
		if(WIFEXITED(status) || WIFSIGNALED(status)){
			active_procs--;
			array[pid2index(pid)].isalive = false;
			signal(SIGUSR2, usr2_handler);
		}
	}
}

static void alrm_handler(UNUSED int sig){
	//if current is not null (i.e we have a process lined up)
	//	if isalive
	//		decrement time
	//		if there's still time
	//			// it can still run
	//			return
	//		send signal to stop the process
	//		enqueue process
	//	set current to NULL
	//while we can dequeue stuff: dequeue and set current to that
	//	if current is not alive
	//		continue
	//	set current time
	//	if current has seen usr1
	//		current->usr1 = false
	//		call usr1 signal on process
	//	else
	//		call sigcont on process
	//return
	
	if(current != NULL){
		if(current->isalive){
			--current->ticks;
			if(current->ticks > 0){
				return;
			}
			kill(current->pid, SIGSTOP);
			q->enqueue(q, ADT_VALUE(current));
		}
		current = NULL;
	}
	while(q->dequeue(q, ADT_ADDRESS(&current))){
		if(!current->isalive){
			// skip dead process
			continue;
		}
		current->ticks = quantum_ticks;
		if(current->usr1){
			current->usr1 = false;
			kill(current->pid, SIGUSR1);
		} else{
			kill(current->pid, SIGCONT);
		}
		break;
	}
	return;
}


int main(int argc, char *argv[])
{
	char *p;
	int val = -1;

	struct itimerval interval;

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
	if(val < MIN_QUANTUM || val > MAX_QUANTUM){
		printf("Unreasonable quantum.\n");
		return 1;
	}

	if(optind >= argc){
		printf("Missing file name.\n");
		exit(EXIT_FAILURE);
	}

	if(signal(SIGCHLD, chld_handler) == SIG_ERR){
		fprintf(stderr, "Can't establish SIGCHLD handler.\n");
		return 1;
	}

	if(signal(SIGALRM, alrm_handler) == SIG_ERR){
		fprintf(stderr, "Can't establish SIGALRM handler.\n");
		return 1;
	}
	

	char *file = argv[optind];
	char buf[BUFSIZ];
	int c;
	int fd = open(file, O_RDONLY);
	int count = 0;

	c = p1getline(fd, buf, BUFSIZ); // this returns 0 if EOF
	
	val = MS_PER_TICK * ((val + 1) / MS_PER_TICK);
	quantum_ticks = val / MS_PER_TICK;

	q = Queue_create(doNothing);

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
	
	signal(SIGCHLD, chld_handler);
	signal(SIGALRM, alrm_handler);
	signal(SIGUSR1, usr1_handler);
	signal(SIGUSR2, usr2_handler);

	active_procs = count;
	num_procs = count;

	for(int i=0; i < count; i++){
		parent = getpid();

		// fork creates copy of self and assigns it an ID
		pid_t pid = fork();

		if(pid<0){
			printf("Unable to fork.\n");
		}
		
		// child
		if(pid == 0){
			pause();	
			// change current program with different program
			execvp(program[i], args[i]);
		}
		// parent
		else{
			usleep(20);
			PCB *pcb = array + i;
			pcb->pid = pid;
			pcb->isalive = true;
			pcb->usr1 = true;
			pcb->ticks = 0;
			q->enqueue(q, ADT_VALUE(pcb));
		}
		
	}
	
	interval.it_value.tv_sec = MS_PER_TICK/1000; //seconds
	interval.it_value.tv_usec = (MS_PER_TICK * 1000) % 1000000; //ms
	interval.it_interval = interval.it_value;
	if(setitimer(ITIMER_REAL, &interval, NULL) == -1){
		printf("Bad timer.\n");
		for(int i = 0; i < count; ++i){
			kill(array[i].pid, SIGKILL);
		}
		goto cleanup;
	}

	alrm_handler(SIGALRM);

	while(active_procs > 0){
		pause();
	}
	goto cleanup;

cleanup:
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
	q->destroy(q);
	return 0;
}
