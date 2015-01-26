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
int processIndex = -1;

// handler code that zeroes out process statistics and exits
void terminate(int signum) {

	if (processIndex == -1) {
		exit(0);
	}

	memory->process[processIndex].pid = 0;
	memory->process[processIndex].countFound = 0;
	//printf("Total numbers processed: %d\n", memory->process[processIndex].countTested + memory->process[processIndex].countSkipped);
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
		return 1;	// perfect
	} else {
		return 0;	// not perfect
	}
}

// loops through bitmap starting at startNumber
int compute(int startNumber) {
	int thisInt, thisBit, testNumber = startNumber;
	int thisPID, wrappedAround = 0;

	// get and attach shared memory segment and message queue
	smid = shmget(SM_KEY, sizeof(sharedMemory), 0);
	if (smid == -1 ) {
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

	// create message that registers compute with manage
	msg message;
	thisPID = getpid();
	message.type = GET_PROCESS_INDEX;
	message.data = thisPID;
	if (msgsnd(mqid, &message, sizeof(message.data), 0) != 0) {
		perror("Process[] index failed");
		exit(1);
	}

	// waits for response message with process statistics array index
	// and verifies that the slot belongs to it
	msgrcv(mqid, &message, sizeof(message.data), PROCESS_INDEX_YES, 0);
	processIndex = message.data;
	if (memory->process[processIndex].pid != thisPID) {
		perror("pids don't match!");
		exit(1);
	}

	// main loop
	while (1) {
		if (testNumber > 1024001) {
			testNumber = 2;
			wrappedAround = 1;
		}
		if (testNumber == startNumber && wrappedAround == 1) {
			// we have looped around the entire bitmap
			terminate(SIGQUIT);
		}

		// get integer index and bit index of bit that represents testNumber
		thisInt = whichInt(testNumber);
		thisBit = whichBit(testNumber);

		// test the number if it has not been tested yet
		// then set the bit and update process statistics
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
		//printf("%d\n", testNumber);
		testNumber++;
	}
}

// entry point
int main(int argc, char *argv[]) {

	// validate number of arguments
	if (argc != 2) {
		printf("usage: ./compute [start number]\n");
		exit(0);
	}

	// validate start number
	int startNumber = atoi(argv[1]);
	if (startNumber > 1024001 || startNumber < 2) {
		printf("Start number x is outside the acceptable range! 2 <= x <= 1,024,001\n");
		exit(0);
	}

	// set up signal handler behavior
	struct sigaction signal;
	memset(&signal, 0, sizeof(signal));
	signal.sa_handler = terminate;
	if (sigaction(SIGINT, &signal, NULL) != 0) {
		perror("SIGINT set failed");
		exit(1);
	}
	if (sigaction(SIGQUIT, &signal, NULL) != 0) {
		perror("SIGQUIT set failed");
		exit(1);
	}
	if (sigaction(SIGHUP, &signal, NULL) != 0) {
		perror("SIGHUP set failed");
		exit(1);
	}

	// begin computation at startNumber
	compute(startNumber);
}
