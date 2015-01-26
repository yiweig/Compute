/* THIS CODE IS MY OWN WORK. IT WAS WRITTEN WITHOUT CONSULTING
 * A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Yiwei Gao */

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "hw4.h"

int smid, mqid;
sharedMemory *memory;

// handler code that terminates running computes
void terminator(int signum) {
	int pid, i;

	// sends a SIGINT signall to all running computes
	for (i = 0; i < PROCESS_CAPACITY; i++) {
		pid = memory->process[i].pid;
		if (pid != 0) {
			if (kill(pid, SIGINT) != 0) {
				perror("Kill process failed");
				exit(1);
			}
		}
	}

	// sleeps 5 seconds, detaches shared memory and marks
	// shared memory and message queue for deletion, then exits
	sleep(5);
	if (shmdt(memory)) {
		perror("Shared memory detach failed");
		exit(1);
	}
	if (shmctl(smid, IPC_RMID, 0)) {
		perror("shared memory IPC_RMID failed");
		exit(1);
	}
	if (msgctl(mqid, IPC_RMID, NULL)) {
		perror("message queue IPC_RMID failed");
	}
	exit(0);
}

// finds free slot in statistics table, or -1 if there are none
int findIndex() {
	int i;
	for (i = 0; i < PROCESS_CAPACITY; i++) {
		if (memory->process[i].pid == 0) {
			return i;
		}
	}
	return -1;
}

// main manage loop
int main(int argc, char *argv[]) {
	int processIndex;
	int perfectIndex = 0;
	
	// create, attach, and zero-out shared memory segment
	// if the shared memory segment already exists, then that means there
	// is another instance of manage running, at which point we must exit
	smid = shmget(SM_KEY, sizeof(sharedMemory), IPC_CREAT | IPC_EXCL | 0666);
	if (smid == -1) {
		perror("There can only be one instance of manage!");
		exit(1);
	}
	memory = shmat(smid, NULL, 0);
	memset(memory->bits, 0, sizeof(memory->bits));
	memset(memory->perfect, 0, sizeof(memory->perfect));
	memset(memory->process, 0, sizeof(memory->process));

	// get message queue
	// if message queu already exists, then that means there is another
	// intance of manage running, at which point we must exit
	mqid = msgget(MQ_KEY, IPC_CREAT | IPC_EXCL | 0666);
	if (mqid == -1) {
		perror("There can only be one instance of manage!");
		exit(1);
	}
	msg message;

	// set up signal behavior
	struct sigaction signal;
	memset(&signal, 0, sizeof(signal));
	signal.sa_handler = terminator;
	if (sigaction(SIGINT, &signal, NULL) != 0) {
		perror("SIGINT failed");
		exit(1);
	}
	if (sigaction(SIGQUIT, &signal, NULL) != 0) {
		perror("SIGQUIT failed");
		exit(1);
	}
	if (sigaction(SIGHUP, &signal, NULL) != 0) {
		perror("SIGHUP failed");
		exit(1);
	}

	// loops and waits for messages
	while (1) {
		// we only want messages of type GET_PROCESS_ID, GET_MANAGE_ID, FOUND_PERFECT_NUM
		// aka "incoming" messages
		msgrcv(mqid, &message, sizeof(message.data), -3, 0);
		//printf("Received message!\n");

		if (message.type == GET_PROCESS_INDEX) {
			//printf("Process index %d requested from pid %d\n", processIndex, message.data);
			// only respond with an index if there is one available,
			// otherwise kill the requesting process
			processIndex = findIndex();
			if (processIndex > -1) {
				memory->process[processIndex].pid = message.data;
				message.type = PROCESS_INDEX_YES;
				message.data = processIndex;
				if (msgsnd(mqid, &message, sizeof(message.data), 0) != 0) {
					perror("Manage message send failed");
					exit(1);
				}
				//processIndex++;
			} else {
				kill(message.data, SIGKILL);
			}
		} else if (message.type == FOUND_PERFECT_NUM) {
			//printf("Perfect number %d received\n", message.data);

			// check if that perfect number has already been found
			int i, found = 0;
			for (i = 0; i < perfectIndex; i++) {
				if (memory->perfect[i] == message.data) {
					found = 1;
					break;
				}
			}
			if (found == 0) {
				memory->perfect[perfectIndex] = message.data;
				perfectIndex++;
			}
		} else if (message.type == GET_MANAGE_PID) {
			message.type = MANAGE_PID_YES;
			message.data = getpid();
			if (msgsnd(mqid, &message, sizeof(message.data), 0) != 0) {
				perror("Manage PID message send failed");
				exit(1);
			}
		}
	}
}
