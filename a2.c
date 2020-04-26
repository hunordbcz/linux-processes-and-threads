#include <stdio.h>
#include <wait.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "a2_helper.h"

#define MAX_PROCESSES 10
#define MAX_THREADS 43
#define MAX_SEMAPHORES 10

typedef enum {
    ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN
} NUMBERS;

typedef struct {
    int ID;
    pthread_t thread;
} THREAD, *pTHREAD;

typedef struct {
    int ID;
    pid_t processes[MAX_PROCESSES];
    int vSemaphore;
} PROCESS, *pPROCESS;

pPROCESS curProc = NULL;

void P(int semId, int semNumber);

void V(int semId, int semNumber);

pPROCESS makeProcess();

int forkProcess(int ID);

void startStop(int processNr, int threadNr);

void waitProcess(int processNr);

void exitProcess(int processNr);

void createThreads(int nrThreads);

void execFOUR(pTHREAD thread);

void execFIVE(pTHREAD thread);

void execSIX(pTHREAD thread);

int main(int argc, char **argv) {
    init();
    info(BEGIN, 1, 0);

    if (forkProcess(2) == 0) { // 2
        startStop(2, 0);
        exit(0);
    } else { // 1
        waitProcess(2);
        if (forkProcess(3) == 0) { // 3
            info(BEGIN, 3, 0);
            if (forkProcess(4) == 0) {
                info(BEGIN, 4, 0);

                createThreads(43);

                if (forkProcess(7)) {
                    waitProcess(7);
                    exitProcess(4);
                }
                startStop(7, 0);
                exit(0);
            } else {
                waitProcess(4);
                if (forkProcess(6) == 0) {
                    info(BEGIN, 6, 0);

                    createThreads(5);

                    exitProcess(6);
                }
                waitProcess(6);
            }
            exitProcess(3);
        } else {
            waitProcess(3);
//            wait(NULL);
            if (forkProcess(5) == 0) {
                info(BEGIN, 5, 0);

                createThreads(5);

                exitProcess(5);
            }
//            wait(NULL);
            waitProcess(5);
        }
    }
    waitProcess(2);
    waitProcess(3);
    waitProcess(6);
    waitProcess(7);
    waitProcess(4);
    waitProcess(5);
    info(END, 1, 0);
    return 0;
}

void *execThread(void *param) {
    pTHREAD thread = (pTHREAD) param;
    switch (curProc->ID) {
        case FOUR:
            execFOUR(thread);
            break;
        case FIVE:

            execFIVE(thread);
            break;
        case SIX:
            execSIX(thread);
            break;
        default:
            break;
    }
    return NULL;
}

void execFOUR(pTHREAD thread) {
    P(curProc->vSemaphore, 0);

    info(BEGIN, curProc->ID, thread->ID);

//    int test = semctl(sem_id43,0,GETVAL);

//    V(sem_id43, 1);
//    P(sem_id43, 1);
//    if (thread->ID == 10){
//        int count = semctl(sem_id43,0,GETVAL);
//        while ((count = semctl(sem_id43,0,GETVAL)) != 5);
//    }

    info(END, curProc->ID, thread->ID);

    V(curProc->vSemaphore, 0);
}

const char *semName = "/af";
const char *semName2 = "/as";

void execFIVE(pTHREAD thread) {
    sem_t *sem_id1 = sem_open(semName, O_CREAT, 0666, 0);
    sem_t *sem_id2 = sem_open(semName2, O_CREAT, 0666, 0);

    P(curProc->vSemaphore, thread->ID - 1);
    if (thread->ID == ONE) {
        printf("WAITING ONE\n");
        if (sem_wait(sem_id1) < 0)
            printf("Child  : [sem_wait1] Failed\n");

    }
    info(BEGIN, curProc->ID, thread->ID);


    if (thread->ID != FIVE) {
        V(curProc->vSemaphore, thread->ID);
    } else {
        V(curProc->vSemaphore, thread->ID - 1);
    }

    P(curProc->vSemaphore, thread->ID - 1);
    info(END, curProc->ID, thread->ID);

    if (thread->ID != ONE) {
        V(curProc->vSemaphore, thread->ID - 2);
    } else {
        if (sem_post(sem_id2) < 0)
            printf("Child   : [sem_post2] Failed \n");
    }
}

void execSIX(pTHREAD thread) {
    sem_t *sem1 = sem_open(semName, O_CREAT, 0666, 0);
    if (sem1 == SEM_FAILED) {
        perror("Parent  : [sem_open] Failed\n");
        return;
    }
    sem_t *sem2 = sem_open(semName2, O_CREAT, 0666, 0);
    if (sem2 == SEM_FAILED) {
        perror("Parent  : [sem_open] Failed\n");
        return;
    }

    if (thread->ID != FOUR) {
        startStop(curProc->ID, thread->ID);

        if (thread->ID == FIVE) {
            if (sem_post(sem1) < 0) printf("Parent   : [sem_post1] Failed \n");
        }


    } else {
        info(BEGIN, curProc->ID, thread->ID);
        if (sem_wait(sem2) < 0)
            printf("Parent  : [sem_wait] Failed\n");
        info(END, curProc->ID, thread->ID);
    }


    if (thread->ID == FOUR) {
        if (sem_close(sem1) != 0 || sem_close(sem2) != 0) {
            perror("Parent  : [sem_close] Failed\n");
            return;
        }
        if (sem_unlink(semName) < 0 || sem_unlink(semName2) < 0) {
            printf("Parent  : [sem_unsemlink] Failed\n");
            return;
        }
    }
}

void createThreads(int nrThreads) {
    THREAD threads[MAX_THREADS];

    switch (curProc->ID) {
        case FOUR:
            semctl(curProc->vSemaphore, 0, SETVAL, 5);
            semctl(curProc->vSemaphore, 1, SETVAL, -4);
            semctl(curProc->vSemaphore, 2, SETVAL, 1);
            break;
        case FIVE:
            semctl(curProc->vSemaphore, 0, SETVAL, 1);
            semctl(curProc->vSemaphore, 1, SETVAL, 0);
            semctl(curProc->vSemaphore, 2, SETVAL, 0);
            semctl(curProc->vSemaphore, 3, SETVAL, 0);
            semctl(curProc->vSemaphore, 4, SETVAL, 0);
            break;
        case SIX:


        default:
            break;
    }


    for (int i = 0; i < nrThreads; i++) {
        threads[i].ID = i + 1;
        if (pthread_create(&threads[i].thread, NULL, execThread, &threads[i]) != 0) {
            perror("Error creating a new thread");
            exit(1);
        }
    }

    for (int i = 0; i < nrThreads; i++) {
        pthread_join(threads[i].thread, NULL);
    }

    semctl(curProc->vSemaphore, 0, IPC_RMID, 0);
    semctl(curProc->vSemaphore, 1, IPC_RMID, 0);
    semctl(curProc->vSemaphore, 2, IPC_RMID, 0);
    semctl(curProc->vSemaphore, 3, IPC_RMID, 0);
    semctl(curProc->vSemaphore, 4, IPC_RMID, 0);
}

void P(int semId, int semNumber) {
    struct sembuf op = {semNumber, -1, 0};
    semop(semId, &op, 1);
}

void V(int semId, int semNumber) {
    struct sembuf op = {semNumber, 1, 0};
    semop(semId, &op, 1);
}

pPROCESS makeProcess() {
    pPROCESS proc = (pPROCESS) malloc(sizeof(PROCESS));
    proc->vSemaphore = semget(IPC_PRIVATE, MAX_SEMAPHORES, IPC_CREAT | 0600);

    if (proc->vSemaphore < 0) {
        perror("Error creating the vSemaphore set");
        exit(EXIT_FAILURE);
    }

    return proc;
}

int forkProcess(int ID) {
    if (curProc == NULL) {
        curProc = makeProcess();
    }

    pid_t pid = fork();
    if (pid == 0) {
        pPROCESS newProc = makeProcess();
        newProc->ID = ID;
        curProc = newProc;
    } else {
        curProc->processes[ID] = pid;
    }
    return pid;
}

void startStop(int processNr, int threadNr) {
    info(BEGIN, processNr, threadNr);
    info(END, processNr, threadNr);
}

void waitProcess(int processNr) {
    wait(NULL);
    waitpid(curProc->processes[processNr], NULL, 0);
}

void exitProcess(int processNr) {
    info(END, processNr, 0);
    exit(0);
}
