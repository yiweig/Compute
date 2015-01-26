/* THIS CODE IS MY OWN WORK. IT WAS WRITTEN WITHOUT CONSULTING
 * A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Yiwei Gao */

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <signal.h>

#include "hw4.h"

int smid, mqid;
sharedMemory *memory;

// only one function since report just does a instantaneous read
int main(int argc, char *argv[]) {

	// get and attach shared memory and message queue, and prepare message
	smid = shmget(SM_KEY, sizeof(sharedMemory), 0);
	if (smid == -1) {
		perror("Shared memory segment does not exist!");
		exit(1);
	}
	memory = shmat(smid, NULL, 0);
	if (memory == (void *) -1) {
		perror("Shared memory segment does not exist!");
		exit(1);
	}
	mqid = msgget(MQ_KEY, 0);
	if (mqid == -1) {
		perror("Message queue does not exist!");
		exit(1);
	}
	msg message;
	int flag = 0;

	// check if "-k" flag was used
	if (argc >= 2) {
		if (strcmp(argv[1], "-k") == 0) {
			//printf("Process will be killed!\n");
			// need to get manage's PID so we can send SIGINT signal to manage
			message.type = GET_MANAGE_PID;
			message.data = 0;
			if (msgsnd(mqid, &message, sizeof(message.data), 0) != 0) {
				perror("Manage PID request failed");
				exit(1);
			}
			msgrcv(mqid, &message, sizeof(message.data), MANAGE_PID_YES, 0);
			//printf("pid from manage acquired\n");
			int targetPID = message.data;
			if (kill(targetPID, SIGINT) != 0) {
				perror("SIGINT send to manager failed");
				exit(1);
			}
			flag = 1;
			//exit(0);
		} else {
			printf("usage: ./report [-k]\n");
			exit(0);
		}
	}

	printf("\nPerfect numbers found: [  ");
	int i, number;
	int totalFound = 0, totalTested = 0, totalSkipped = 0;
	for (i = 0; i < PERFECT_CAPACITY; i++) {
		number = memory->perfect[i];
		if (number != 0) {
			printf("%d  ", number);
		}

		//totalFound   += memory->process[i].countFound;
		//totalTested  += memory->process[i].countTested;
		//totalSkipped += memory->process[i].countSkipped;
	}
	printf("]\n\n");

	int integer, bit;

	for (integer = 0; integer < NUMBER_OF_INTS; integer++) {
		if (memory->bits[integer] == -1) {
			totalTested += 32;
		} else {
			for (bit = 0; bit < 32; bit++) {
				if ((memory->bits[integer] & (1 << bit)) == 1) {
					totalTested++;
				}
			}
		}
	}


	//printf("Total number found:   %d\n", totalFound);
	printf("Total number tested:  %d\n", totalTested);
	//printf("Total number skipped: %d\n", totalSkipped);

	printf("\n");

	// format and print out the process statistics data
	//printf("index\t\tpid\t\tfound\t\ttested\t\tskipped\n");
	int thePID;
	printf("Currently running compute pids:\n");
	for (i = 0; i < PROCESS_CAPACITY; i++) {
		thePID = memory->process[i].pid;
		if (thePID != 0 && flag == 0) {
			printf("%d  ", thePID);
		}

		/*
		if (memory->process[i].pid != 0) {
			printf("%d\t\t%d\t\t%d\t\t%d\t\t%d\n", i, 
			memory->process[i].pid, 
			memory->process[i].countFound,
			memory->process[i].countTested, 
			memory->process[i].countSkipped);
		} */
	}
	printf("\n\n");
}
