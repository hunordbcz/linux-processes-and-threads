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

#define MAX_THREADS 43
#define MAX_SEMAPHORES 5

const char *SIX_FIRST = "/proc_six__thread_five";
const char *FIVE_FIRST = "/proc_five__thread_one";
const char *SIX_SECOND = "/proc_six__thread_four";
sem_t *semSixFive;
sem_t *semFiveOne;
sem_t *semSixFour;

typedef enum {
    ONE = 1, TWO, THREE, FOUR, FIVE, SIX
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
    int threadTenEntered;
    int threadTenExited;
    int threadsExited;
    int nrRunning;
    int total;
    pthread_mutex_t lock;
    pthread_cond_t condExit;
    pthread_cond_t condWait;
    pthread_cond_t condTen;
} PROCESS, *pPROCESS;

pPROCESS curProc = NULL;

int forkProcess(int ID, int waitChild);

pPROCESS makeProcess();

void exitProcess();

void exitThread(pTHREAD thread);

void createThreads(int nrThreads);

void *execThread(void *param);

void execFOUR(pTHREAD thread);

void execFIVE(pTHREAD thread);

void execSIX(pTHREAD thread);

void initializeSemaphoreV();

void initializeMutex();

void startStop(int processNr, int threadNr);

void P(int semId, int semNumber);

void V(int semId, int semNumber);

void semPost(sem_t *sem);

void semWait(sem_t *sem);

void lock(pthread_mutex_t *lock);

void unlock(pthread_mutex_t *lock);

void clearGlobalSemaphore();

void clearSemaphoreV(int nr);

void clearMutex();

int main(int argc, char **argv) {
    init();
    semSixFive = sem_open(SIX_FIRST, O_CREAT, 0666, 0);
    semFiveOne = sem_open(FIVE_FIRST, O_CREAT, 0666, 0);
    semSixFour = sem_open(SIX_SECOND, O_CREAT, 0666, 0);

    if (semSixFive == SEM_FAILED || semFiveOne == SEM_FAILED || semSixFour == SEM_FAILED) {
        perror("sem_open Failed at main()\n");
        exit(EXIT_FAILURE);
    }

    info(BEGIN, 1, 0);
    if (forkProcess(2, TRUE) == CHILD) {
        exitProcess(2);
    } else {
        if (forkProcess(3, FALSE) == CHILD) {
            if (forkProcess(4, TRUE) == CHILD) {
                createThreads(43);

                if (forkProcess(7, TRUE) == CHILD) {
                    exitProcess(7);
                }
                exitProcess(4);
            } else {
                if (forkProcess(6, TRUE) == CHILD) {
                    createThreads(5);
                    exitProcess(6);
                } else {
                    exitProcess(3);
                }
            }
        } else {
            if (forkProcess(5, TRUE) == CHILD) {
                createThreads(5);
                exitProcess(5);
            } else {
                wait(NULL); // wait for 3
            }
        }
    }

    info(END, 1, 0);
    return 0;
}

int forkProcess(int ID, int waitChild) {
    if (curProc == NULL) {
        curProc = makeProcess();
    }

    pid_t pid = fork();
    switch (pid) {
        case -1:
            perror("Cannot create new process at forkProcess()");
            exit(EXIT_FAILURE);
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

pPROCESS makeProcess() {
    pPROCESS proc = (pPROCESS) malloc(sizeof(PROCESS));
    proc->processes = (pid_t *) malloc(sizeof(pid_t));
    proc->threadTenEntered = FALSE;
    proc->threadTenExited = FALSE;
    proc->threadsExited = 0;
    proc->nrRunning = 0;
    proc->total = 0;
    return proc;
}

void exitProcess() {
    info(END, curProc->ID, 0);
    if (curProc != NULL) {
        if (curProc->processes != NULL) {
            free(curProc->processes);
        }
        free(curProc);
    }

    exit(EXIT_SUCCESS);
}

void exitThread(pTHREAD thread) {
    info(END, curProc->ID, thread->ID);

    if (thread->ID == 10) {
        curProc->threadTenExited = TRUE;
        curProc->threadTenEntered = FALSE;
        pthread_cond_broadcast(&curProc->condExit);
    }
    curProc->nrRunning--;
    curProc->threadsExited++;
}

void createThreads(int nrThreads) {
    THREAD threads[MAX_THREADS];
    switch (curProc->ID) {
        case FOUR:
            initializeSemaphoreV();
            initializeMutex();
            semctl(curProc->vSemaphore, 0, SETVAL, 5);
            break;
        case FIVE:
            initializeSemaphoreV();
            semctl(curProc->vSemaphore, 0, SETVAL, 1);
            semctl(curProc->vSemaphore, 1, SETVAL, 0);
            semctl(curProc->vSemaphore, 2, SETVAL, 0);
            semctl(curProc->vSemaphore, 3, SETVAL, 0);
            semctl(curProc->vSemaphore, 4, SETVAL, 0);
            break;
        default:
            break;
    }

    for (int i = 0; i < nrThreads; i++) {
        threads[i].ID = i + 1;
        if (pthread_create(&threads[i].thread, NULL, execThread, &threads[i]) != 0) {
            perror("Error creating a new thread at createThreads()");
            exit(1);
        }
    }

    for (int i = 0; i < nrThreads; i++) {
        pthread_join(threads[i].thread, NULL);
    }

    switch (curProc->ID) {
        case FOUR:
            clearMutex();
            clearSemaphoreV(1);
            break;
        case FIVE:
            clearSemaphoreV(5);
            break;
        case SIX:
            clearGlobalSemaphore();
        default:
            break;
    }
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

        lock(&curProc->lock);
            curProc->total++;
            curProc->nrRunning++;
        unlock(&curProc->lock);

        lock(&curProc->lock);
            while (thread->ID != 10 && !curProc->threadTenEntered && !curProc->threadTenExited && curProc->total > 38) {
                pthread_cond_wait(&curProc->condWait, &curProc->lock);
            }
        unlock(&curProc->lock);

        lock(&curProc->lock);
            if (thread->ID == 10) {
                curProc->threadTenEntered = 1;
                pthread_cond_broadcast(&curProc->condWait);
            } else {
                pthread_cond_signal(&curProc->condTen);
            }
        unlock(&curProc->lock);

        lock(&curProc->lock);
            while (thread->ID != 10 && curProc->threadTenEntered && !curProc->threadTenExited) {
                pthread_cond_wait(&curProc->condExit, &curProc->lock);
            }
            while (thread->ID == 10 && curProc->nrRunning < 5) {
                pthread_cond_wait(&curProc->condTen, &curProc->lock);
            }
        unlock(&curProc->lock);

        lock(&curProc->lock);
            exitThread(thread);
        unlock(&curProc->lock);
    V(curProc->vSemaphore, 0);
}

void execFIVE(pTHREAD thread) {
    P(curProc->vSemaphore, thread->ID - 1);

    if (thread->ID == 1) {
        semPost(semSixFive);
        semWait(semFiveOne);
    }

    info(BEGIN, curProc->ID, thread->ID);

    // 1->2->..->2->1
    if (thread->ID != FIVE) {
        V(curProc->vSemaphore, thread->ID);
    } else {
        V(curProc->vSemaphore, thread->ID - 1);
    }

    P(curProc->vSemaphore, thread->ID - 1);
    exitThread(thread);

    if (thread->ID != ONE) {
        V(curProc->vSemaphore, thread->ID - 2);
    } else {
        semPost(semSixFour);
    }
}

void execSIX(pTHREAD thread) {
    if (thread->ID == FOUR) {
        semWait(semSixFour);
    }

    startStop(curProc->ID, thread->ID);

    if (thread->ID == FIVE) {
        semPost(semFiveOne);
    }
}

void initializeSemaphoreV() {
    curProc->vSemaphore = semget(IPC_PRIVATE, MAX_SEMAPHORES, IPC_CREAT | 0600);
    if (curProc->vSemaphore < 0) {
        perror("Error creating the vSemaphore set at initializeSemaphoreV()");
        exit(EXIT_FAILURE);
    }
}

void initializeMutex() {
    if (pthread_mutex_init(&curProc->lock, NULL) != 0) {
        perror("Cannot initialize lock at initializeMutex()");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_init(&curProc->condWait, NULL) != 0 || pthread_cond_init(&curProc->condTen, NULL) != 0 ||
        pthread_cond_init(&curProc->condExit, NULL) != 0) {
        perror("Cannot initialize the condition variables at initializeMutex()");
        exit(EXIT_FAILURE);
    }
}

void startStop(int processNr, int threadNr) {
    info(BEGIN, processNr, threadNr);
    info(END, processNr, threadNr);
}

void P(int semId, int semNumber) {
    struct sembuf op = {semNumber, -1, 0};
    if (semop(semId, &op, 1) < 0) {
        perror("semop failed at P()");
        exit(EXIT_FAILURE);
    }
}

void V(int semId, int semNumber) {
    struct sembuf op = {semNumber, 1, 0};
    if (semop(semId, &op, 1) < 0) {
        perror("semop failed at V()");
        exit(EXIT_FAILURE);
    }
}

void semPost(sem_t *sem) {
    if (sem_post(sem) < 0) {
        perror("sem_post failed at semPost()");
        exit(EXIT_FAILURE);
    }
}

void semWait(sem_t *sem) {
    if (sem_wait(sem) < 0) {
        perror("sem_wait failed at semWait()");
        exit(EXIT_FAILURE);
    }
}

void lock(pthread_mutex_t *lock) {
    if (pthread_mutex_lock(lock) != 0) {
        perror("Cannot take lock at lock()");
        exit(EXIT_FAILURE);
    }
}

void unlock(pthread_mutex_t *lock) {
    if (pthread_mutex_unlock(lock) != 0) {
        perror("Cannot release lock at unlock()");
        exit(EXIT_FAILURE);
    }
}

void clearGlobalSemaphore() {
    if (sem_close(semSixFive) != 0 || sem_close(semFiveOne) != 0 || sem_close(semSixFour) != 0) {
        perror("Semaphore Close Failed at clearGlobalSemaphore");
        exit(EXIT_FAILURE);
    }
    if (sem_unlink(SIX_FIRST) < 0 || sem_unlink(FIVE_FIRST) < 0 || sem_unlink(SIX_SECOND) < 0) {
        perror("Semaphore Unlink Failed at clearGlobalSemaphore");
        exit(EXIT_FAILURE);
    }
}

void clearSemaphoreV(int nr) {
    for (int i = 0; i < nr; i++) {
        semctl(curProc->vSemaphore, i, IPC_RMID, 0);
    }
}

void clearMutex() {
    if (pthread_mutex_destroy(&curProc->lock) != 0) {
        perror("Cannot destroy lock at clearMutex()");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_destroy(&curProc->condWait) != 0 || pthread_cond_destroy(&curProc->condTen) != 0 ||
        pthread_cond_destroy(&curProc->condExit) != 0) {
        perror("Cannot destroy condition variable at clearMutex()");
        exit(EXIT_FAILURE);
    }
}