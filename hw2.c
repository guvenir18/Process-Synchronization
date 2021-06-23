/*
    İlke Anıl Güvenir
    150180042
    Note: Please compile with c99 and -D_SVID_SOURCE flag
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>

// Keys for semaphores and shared memory
int SH_KEY;
int SEM_KEY;
int SEM_KEY_2;

// Initialize keys for semaphores and shared memory
void initKeys(char *argv[])
{
    char *keyString = malloc(strlen(argv[0]) + 1);
    strcat(keyString, argv[0]);
    SEM_KEY = ftok(keyString, 2);
    SH_KEY = ftok(keyString, 3);
    SEM_KEY_2 = ftok(keyString, 4);
    free(keyString);
}

void sem_signal(int semid, int val)
{
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = val;
    semaphore.sem_flg = 0;   
    semop(semid, &semaphore, 1);
}

void sem_wait(int semid, int val)
{
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = (-1*val);
    semaphore.sem_flg = 0;  
    semop(semid, &semaphore, 1);
}

int main(int argc, char* argv[]) {
    initKeys(argv);
    // This semaphore used to synchronize child and parent processes
    int sem_id = semget(SEM_KEY, 1, 0700|IPC_CREAT);
    semctl(sem_id, 0, SETVAL, 0);
    // This semaphore used to synchronize childs.
    int sem_id_2 = semget(SEM_KEY_2, 1, 0700|IPC_CREAT);
    semctl(sem_id_2, 0, SETVAL, 0);

    FILE* fp = fopen("input.txt", "r");
    int M, n, f, i;
    int childs[2];
    fscanf(fp, "%d %d ", &M, &n);
    int shmid = shmget(SH_KEY, sizeof(int) * (2 * n + 4), 0700|IPC_CREAT);
    int* shm = (int*)shmat(shmid, NULL, 0);
    shm[0] = n;
    shm[1] = M;
    shm[2] = 0; // x
    shm[3] = 0; // y
    for (int i = 0; i < n; i++) {
        int temp;
        fscanf(fp, "%d ", &temp);
        shm[i+4] = temp;
    }
    shmdt(shm);
    fclose(fp);
    for (i = 0; i < 2; i++) {
        f = fork();
        if (f == 0) {
            break;
        }
        childs[i] = f;
    }
    // Parent Process
    if (f > 0) {
        sem_wait(sem_id, 2);
        shmid = shmget(SH_KEY, sizeof(int) * (2 * n + 4), 0);
        shm = (int*)shmat(shmid, NULL, 0);
        fp = fopen("output.txt", "w");
        int x = shm[2];
        int y = shm[3];
        fprintf(fp, "%d\n%d\n", shm[1], shm[0]);      
        for (int k = 0; k < n; k++) {
            fprintf(fp, "%d ", shm[k + 4]);
        }
        fprintf(fp, "\n%d\n", x);
        for (int k = 0; k < x; k++) {
            fprintf(fp, "%d ", shm[k + 4 + n]);
        }
        fprintf(fp, "\n%d\n", y);
        for (int k = 0; k < y; k++) {
            fprintf(fp, "%d ", shm[k + 4 + n + x]);
        }
        shmdt(shm);
        shmctl(shmid, IPC_RMID, 0);
        semctl(sem_id, 0, IPC_RMID, 0);
        semctl(sem_id_2, 0, IPC_RMID, 0);
        fclose(fp);
        exit(0);
    } 
    // Child processes
    else {
        // First Child (<= M and array B)
        if (i == 0) {
            // Counting of x starts.
            shmid = shmget(SH_KEY, sizeof(int) * (2 * n + 4), 0);
            shm = (int*)shmat(shmid, NULL, 0);
            for (int k = 0; k < n; k++) {
                if (shm[k + 4] <= shm[1]) {
                    shm[2] += 1;
                }
            }
            shmdt(shm);
            // Counting of x done.
            sem_signal(sem_id_2, 1);
            // Start copying values <= M into array B
            shmid = shmget(SH_KEY, sizeof(int) * (2 * n + 4), 0);
            shm = (int*)shmat(shmid, NULL, 0);
            int c = 0;
            for (int k = 0; k < n; k++) {
                if (shm[k + 4] <= M) {
                    shm[c + 4 + n] = shm[k + 4];
                    c++;
                }
            }
            shmdt(shm);
            sem_signal(sem_id, 1);
            exit(0);
        }
        // Second Child (> M and array C)
        else if (i == 1) {
            // Counting of y starts.
            shmid = shmget(SH_KEY, sizeof(int) * (2 * n + 4), 0);
            shm = (int*)shmat(shmid, NULL, 0);
            for (int k = 0; k < n; k++) {
                if (shm[k + 4] > shm[1]) {
                    shm[3] += 1;
                }
            }
            shmdt(shm);
            // Counting of y done.
            // Wait until counting of x completed.
            sem_wait(sem_id_2, 1);
            shmid = shmget(SH_KEY, sizeof(int) * (2 * n + 4), 0);
            shm = (int*)shmat(shmid, NULL, 0);
            int x = shm[2];
            int c = 0;
            for (int k = 0; k < n; k++) {
                if (shm[k + 4] > M) {
                    shm[c + 4 + n + x] = shm[k + 4];
                    c++;
                }
            }
            shmdt(shm);
            sem_signal(sem_id, 1);
            exit(0);
        }
    }
    return 0;
}