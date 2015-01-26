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

int smid;
sharedMemory *memory;

// main manage loop
int main(int argc, char *argv[]) {
	smid = shmget(SM_KEY, sizeof(sharedMemory), 0);
	if (smid == -1) {
		perror("There can only be one instance of manage!");
		exit(1);
	}
	memory = shmat(smid, NULL, 0);
	memset(memory->bits, -1, sizeof(memory->bits));
	memset(memory->perfect, 0, sizeof(memory->perfect));
	memset(memory->process, 0, sizeof(memory->process));
}
