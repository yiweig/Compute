/* THIS CODE IS MY OWN WORK. IT WAS WRITTEN WITHOUT CONSULTING
 * A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Yiwei Gao */

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "hw4.h"

int smid, mqid;
sharedMemory *memory;
//int processIndex;
int processIndex;

void terminate(int signum) {
	/*
	int index = getpid(), i;

	for (i = 0; i < sizeof(memory->process) / sizeof(stats); i++) {
		if (memory->process[i].pid == index) {
			index = i;
			break;
		}
	}
	*/
	memory->process[processIndex].pid = 0;
	memory->process[processIndex].countFound = 0;
	memory->process[processIndex].countTested = 0;
	memory->process[processIndex].countSkipped = 0;

	exit(0);
}

// determines index of the integer that contains the bit that represents j
int whichInt(int j) {
	return ((j - 2) / 32);
}

// determines which bit represents j
int whichBit(int j) {
	return ((j - 2) % 32);
}

// test if a number is perfect
int test(int testNumber) {
	int sumSoFar = 0, sumOfDivisors = 0, i;

	for (i = 1; i < testNumber; i++) {
		if (testNumber % i == 0) {
			sumSoFar += i;
		}
	}
	sumOfDivisors = sumSoFar;

	if (sumOfDivisors == testNumber) {
		return 1;
	} else {
		return 0;
	}
}

int compute(int startNumber) {
	int thisInt, thisBit, testNumber = startNumber;
	int pid, wrappedAround = 0;

	//int smid, mqid;
	//sharedMemory *memory;
	smid = shmget(SM_KEY, sizeof(sharedMemory), 0);
	memory = shmat(smid, NULL, 0);

	mqid = msgget(MQ_KEY, 0);

	msg message;
	pid = getpid();
	message.type = GET_PROCESS_INDEX;
	message.data = pid;
	if (msgsnd(mqid, &message, sizeof(message.data), 0) != 0) {
		perror("Process[] index failed");
		exit(1);
	}

	msgrcv(mqid, &message, sizeof(message.data), PROCESS_INDEX_YES, 0);
	processIndex = message.data;
	//processIndex = index;
	if (memory->process[processIndex].pid != pid) {
		perror("pids don't match!");
		exit(1);
	}

	while (1) {
		if (testNumber == startNumber && wrappedAround == 1) {
			// we have looped around the entire bitmap
			exit(1);
		}
		if (testNumber > 1024001) {
			testNumber = 2;
			wrappedAround = 1;
		}
		thisInt = whichInt(testNumber);
		thisBit = whichBit(testNumber);

		if ((memory->bits[thisInt] & (1 << thisBit)) == 0) {
			if (test(testNumber)) {
				// tell manager that testNumber is perfect
				message.type = FOUND_PERFECT_NUM;
				message.data = testNumber;
				if (msgsnd(mqid, &message, sizeof(message.data), 0) != 0) {
					perror("Perfect number send failed");
					exit(1);
				}
				memory->process[processIndex].countFound++;
			} 
			memory->bits[thisInt] |= (1 << thisBit);
			memory->process[processIndex].countTested++;
		} else {
			memory->process[processIndex].countSkipped++;
		}
		testNumber++;
	}
}

int main(int argc, char *argv[]) {

	int startNumber = atoi(argv[1]);

	if (startNumber > 1024001 || startNumber < 2) {
		printf("Start number x is outside the acceptable range! 2 <= x <= 1,024,001\n");
		exit(0);
	}

	struct sigaction signal;
	memset(&signal, 0, sizeof(signal));
	signal.sa_handler = terminate;
	if (sigaction(SIGINT, &signal, NULL)) {
		perror("SIGINT set failed");
		exit(1);
	}
	if (sigaction(SIGQUIT, &signal, NULL)) {
		perror("SIGQUIT set failed");
		exit(1);
	}
	if (sigaction(SIGHUP, &signal, NULL)) {
		perror("SIGHUP set failed");
		exit(1);
	}

	compute(startNumber);

}
