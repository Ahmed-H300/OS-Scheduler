#include <stdio.h> //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300

///==============================
// don't mess with this variable//
int *shmaddr; //
//===============================

int getClk() {
    return *shmaddr;
}

/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
 */
void initClk() {
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int) shmid == -1) {
        // Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *) shmat(shmid, (void *) 0, 0);
}

/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
 */

void destroyClk(bool terminateAll) {
    shmdt(shmaddr);
    if (terminateAll) {
        /**
         * @note Atta
         * this is because the parent( process generator ) and all the children share
         * the same group id so this will end all the processes if called
         */

        killpg(getpgrp(), SIGINT);
    }
}

/**
 * @brief this is the struct that should be sent from the process_generator to the scheduler
 *  @note msgsend and ,sgresv are the same except the size
 *  size = sizeof(process_struct) - sizeof(mtype)
 *  @note mtype for coming processes = 1
 */

struct process_struct {
    long mtype; // 1
    int id;
    int arrival;
    int runtime;
    int priority;
};

struct chosen_algorithm {
    long mtype; // 2
    short algo; // 1 for RR
    int arg; // quantum of RR algorithm
};






