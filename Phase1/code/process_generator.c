#include "headers.h"
#include <string.h>

#define FILE_NAME_LENGTH 100

// message queue id
int msgq_id;

void clearResources(int);

int NumOfProcesses(FILE *file);

void ReadProcessesData(FILE *file, struct process_struct Processes[], int ProcessesNum);

void Create_Scheduler_Clk(int *sch_pid, int *clk_pid);

int GetSyncProcessesNum(int Time, struct process_struct Processes[], int ProcessesNum, int ProcessIterator);

int main(int argc, char *argv[]) {
    signal(SIGINT, clearResources);

    // 1. Read the input files.
    //make an array of processes and store data of each process in it
    char FileName[FILE_NAME_LENGTH] = "processes.txt";
    // strcpy(FileName, argv[1]);
    FILE *file = fopen(FileName, "r");
    int ProcessesNum = NumOfProcesses(file);
    // make an array of processes and store data of each process in it
    struct process_struct Processes[ProcessesNum];
    ReadProcessesData(file, Processes, ProcessesNum);
    fclose(file);

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    struct chosen_algorithm Initializer;
    Initializer.algo = atoi(argv[1]);
    Initializer.arg = 6;
    if (Initializer.algo == 1) //RR
    {
        Initializer.arg = atoi(argv[2]);
    }
    Initializer.NumOfProcesses = ProcessesNum;
    Initializer.mtype = ALGO_TYPE;
    int sch_pid, clk_pid, stat_loc;
    Create_Scheduler_Clk(&sch_pid, &clk_pid);
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this getClk();

    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // create a message queue
    // create the queue id
    key_t qid = ftok("keyfile", 'q');
    msgq_id = msgget(qid, 0666 | IPC_CREAT);
    if (msgq_id == -1) {
        perror("error has been occurred in the message queue\n");
        exit(-1);
    }


    // 6. Send the information to the scheduler at the appropriate time.
    // send the initiator struct to the scheduler
    int send_val = msgsnd(msgq_id, &Initializer, sizeof(Initializer) - sizeof(Initializer.mtype), !IPC_NOWAIT);
    if (send_val == -1) {
        perror("error has been occurred in scheduler initiation operation\n");
        exit(-1);
    }
    // secondly, sending processes in the appropriate time
    int ProcessIterator = 0;
    int prevClk = -1;
//    printf("from Gen: sent message to scheduler at %d\n", getClk());
    while (ProcessIterator < ProcessesNum) {
        prevClk = getClk();
        //get number of processes to be sent to the scheduler
        int count = GetSyncProcessesNum(prevClk, Processes, ProcessesNum, ProcessIterator);
        //send the number to the scheduler
        struct count_msg c = {10, count};
        send_val = msgsnd(msgq_id, &c, sizeof(int), !IPC_NOWAIT);
        if (send_val == -1) {
            perror("error has been occurred while sending the number of processes/n");
        }
        //send the processes to the scheduler
        while (count > 0 && prevClk >= Processes[ProcessIterator].arrival) {
            // send the process to the scheduler
            send_val = msgsnd(msgq_id, &Processes[ProcessIterator],
                              sizeof(Processes[ProcessIterator]) - sizeof(Processes[ProcessIterator].mtype),
                              !IPC_NOWAIT);
            if (send_val == -1) {
                perror("error has been occurred while sending to the scheduler\n");
            }
            ProcessIterator++;
            count--;
        }
        printf("\nfrom gen %d %d\n", ProcessIterator, ProcessesNum);
        while (prevClk == getClk());

    }
    
    sleep(1); // to give time for scheduler to run
    printf("From Gen: Done Send process\n");
    kill(sch_pid, SIGUSR1); //sent all
    int st;
    // wait for clk and scheduler
    wait(&st);
    wait(&st);
}

void clearResources(int signum) {
    // TODO Clears all resources in case of interruption
    msgctl(msgq_id, IPC_RMID, (struct msqid_ds *) 0);
    kill(getpgrp(), SIGKILL);
    //wait for clk and scheduler
    exit(0);
    signal(SIGINT, clearResources);
}

// remember to add the file name in it
int NumOfProcesses(FILE *file) {
    if (file == NULL) {
        perror("the file does not exist");
        exit(-1);
    }
    // note : do not count any line stating with #
    int count = 0;
    int id, arrive, runtime, priority;
    for (char ch = getc(file); ch != EOF; ch = getc(file)) {
        if (ch == '\n') {
            break;
        }
    }
    while (fscanf(file, "%d %d %d %d", &id, &arrive, &runtime, &priority) == 4) {
        count++;
    }
    return count;
}

void ReadProcessesData(FILE *file, struct process_struct Processes[], int ProcessesNum) {
    // note that the pointer of the file points to the end of it
    // because we have count the number of processes so we need to move it again to the begining of the file
    fseek(file, 0, SEEK_SET);
    // skip the first line because it is a comment
    char dummy[10];
    for (int i = 0; i < 4; i++) {
        fscanf(file, "%s", dummy);
    }
    for (int i = 0; i < ProcessesNum; i++) {
        Processes[i].mtype = 1;
        fscanf(file, "%d", &Processes[i].id);
        fscanf(file, "%d", &Processes[i].arrival);
        fscanf(file, "%d", &Processes[i].runtime);
        fscanf(file, "%d", &Processes[i].priority);
    }
}

void Create_Scheduler_Clk(int *sch_pid, int *clk_pid) {
    *sch_pid = fork();

    if (*sch_pid == 0) {
        execl("./scheduler.out", "./scheduler.out", NULL);
    } else if (*sch_pid == -1) {
        printf("error has been occured while initiating the scheduler\n");
        exit(-1);
    }
    *clk_pid = fork();

    if (*clk_pid == 0) {
        execl("./clk.out", "./clk.out", NULL);
    } else if (*clk_pid == -1) {
        printf("error has been occured while initiating the clock\n");
        exit(-1);
    }
}

int GetSyncProcessesNum(int Time, struct process_struct *Processes, int ProcessesNum, int curr_iter) {
    int count = 0;
    for (int i = curr_iter; i < ProcessesNum; i++) {
        if (Processes[i].arrival <= Time) {
            count++;
        }
        if (Processes[i].arrival > Time) {
            break;
        }
    }
    return count;
}