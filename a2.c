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

#define MAX_PROCESSES 8
#define MAX_THREADS 43
#define MAX_SEMAPHORES 10

typedef enum {
    ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN
} NUMBERS;

typedef enum {
    FALSE, TRUE
} BOOLEAN;

typedef enum {
    CHILD, PARENT
} PROCESS_TYPE;

typedef struct {
    int ID;
    pthread_t thread;
} THREAD, *pTHREAD;

typedef struct {
    int ID;
    pid_t *processes;
    int vSemaphore;
} PROCESS, *pPROCESS;

pPROCESS curProc = NULL;

void P(int semId, int semNumber);

void V(int semId, int semNumber);

pPROCESS makeProcess();

int forkProcess(int ID, int waitChild);

void startStop(int processNr, int threadNr);

void waitProcess(int processNr);

void exitProcess(int processNr);

void createThreads(int nrThreads);

void execFOUR(pTHREAD thread);

void execFIVE(pTHREAD thread);

void execSIX(pTHREAD thread);

const char *SIX_FIRST = "/6_first_done";
const char *FIVE_FIRST = "/5_first_done";
const char *SIX_SECOND = "/6_second_done";

void clear() {
    sem_t *semSixOne = sem_open(SIX_FIRST, O_CREAT, 0666, 0);
    sem_t *semFiveOne = sem_open(FIVE_FIRST, O_CREAT, 0666, 0);
    sem_t *semSixTwo = sem_open(SIX_SECOND, O_CREAT, 0666, 0);

    if (semSixOne == SEM_FAILED || semFiveOne == SEM_FAILED || semSixTwo == SEM_FAILED) {
        perror("Parent  : [sem_open] Failed\n");
        exit(10);
    }
    if (sem_close(semSixOne) != 0 || sem_close(semFiveOne) != 0 || sem_close(semSixTwo) != 0) {
        perror("Parent  : [sem_close] Failed\n");
        exit(10);
    }
    if (sem_unlink(SIX_FIRST) < 0 || sem_unlink(FIVE_FIRST) < 0 || sem_unlink(SIX_SECOND) < 0) {
        printf("Parent  : [sem_unsemlink] Failed\n");
        exit(10);
    }
}

int main(int argc, char **argv) {


    init();
    sem_t *semSixTwo = sem_open(SIX_SECOND, O_CREAT, 0666, 0);

    if (semSixTwo == SEM_FAILED) {
        perror("Parent  : [sem_open] Failed\n");
        return -1;
    }

    info(BEGIN, 1, 0);

    if (forkProcess(2, TRUE) == CHILD) {
        exitProcess(2);
    } else {
        if (forkProcess(3, FALSE) == CHILD) {
            if (forkProcess(4, TRUE) == CHILD) {
                createThreads(5); // 43 normally
                if (forkProcess(7, TRUE) == CHILD) {
                    exitProcess(7);
                } else {
                    exitProcess(4);
                }
            } else {
                if (forkProcess(6, TRUE) == CHILD) {
                    createThreads(5);
                    exitProcess(6);
                    sem_post(semSixTwo);
                }
            }
        } else {
            if (forkProcess(5, TRUE) == CHILD) {
                createThreads(5);
                exitProcess(5);
            } else {
                sem_wait(semSixTwo);
                wait(NULL); // wait for Proc6
                wait(NULL); // wait for Proc6
                wait(NULL); // wait for Proc3

            }
        }
    }
//    waitProcess(2);
//    waitProcess(3);
//    waitProcess(6);
//    waitProcess(7);
//    waitProcess(4);
//    waitProcess(5);

    info(END, 1, 0);
    clear();
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

void execFIVE(pTHREAD thread) {
    sem_t *semSixOne = sem_open(SIX_FIRST, O_CREAT, 0666, 0);
    sem_t *semFiveOne = sem_open(FIVE_FIRST, O_CREAT, 0666, 0);

    P(curProc->vSemaphore, thread->ID - 1);
    if (thread->ID == ONE) {
        printf("WAITING ONE\n");
        if (sem_wait(semSixOne) < 0)
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
        if (sem_post(semFiveOne) < 0)
            printf("Child   : [sem_post2] Failed \n");
    }
}

void execSIX(pTHREAD thread) {
    sem_t *semSixOne = sem_open(SIX_FIRST, O_CREAT, 0666, 0);
    sem_t *semFiveOne = sem_open(FIVE_FIRST, O_CREAT, 0666, 0);
    sem_t *semSixTwo = sem_open(SIX_SECOND, O_CREAT, 0666, 0);

    if (semSixOne == SEM_FAILED || semFiveOne == SEM_FAILED || semSixTwo == SEM_FAILED) {
        perror("Parent  : [sem_open] Failed\n");
        return;
    }

    startStop(curProc->ID, thread->ID);

    if (thread->ID != FOUR) {
        startStop(curProc->ID, thread->ID);

        if (thread->ID == FIVE) {
            if (sem_post(semSixOne) < 0) printf("Parent   : [sem_post1] Failed \n");
        }


    } else {
        info(BEGIN, curProc->ID, thread->ID);
        if (sem_wait(semFiveOne) < 0)
            printf("Parent  : [sem_wait] Failed\n");
        info(END, curProc->ID, thread->ID);
        if (sem_post(semSixTwo) < 0) printf("Parent   : [sem_post1] Failed \n");
    }


    if (thread->ID == FOUR) {

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

    proc->processes = (pid_t *) malloc(sizeof(pid_t));

    return proc;
}

int forkProcess(int ID, int waitChild) {
    if (curProc == NULL) {
        curProc = makeProcess();
    }

    pid_t pid = fork();
    switch (pid) {
        case -1:

            exit(-1);
        case 0:
            info(BEGIN, ID, 0);
            pPROCESS newProc = makeProcess();
            newProc->ID = ID;
            curProc = newProc;

            return CHILD;
        default:
            curProc->processes[ID] = pid;
            if (waitChild) {
                waitpid(pid, NULL, 0);
            }
            return PARENT;
    }


}

void startStop(int processNr, int threadNr) {
    info(BEGIN, processNr, threadNr);
    info(END, processNr, threadNr);
}

void waitProcess(int processNr) {
    waitpid(curProc->processes[processNr], NULL, 0);
}

void exitProcess(int processNr) {
    info(END, processNr, 0);
    exit(0);
}
