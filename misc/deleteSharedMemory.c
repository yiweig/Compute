/* THIS CODE IS MY OWN WORK. IT WAS WRITTEN WITHOUT CONSULTING
 * A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Yiwei Gao */

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <string.h>

#include "hw4.h"

int smid, mqid;
sharedMemory *memory;

int main(int argc, char *argv[]) {

	smid = shmget(SM_KEY, sizeof(sharedMemory), 0);
	if (shmctl(smid, IPC_RMID, 0)) {
		printf("shared memory remove failed\n");
	}
	mqid = msgget(MQ_KEY, 0);
	if (msgctl(mqid, IPC_RMID, NULL)) {
		printf("message queue remove failed\n");
	}

	printf("smid %d deleted\n", smid);
	printf("mqid %d deleted\n", mqid);

}
