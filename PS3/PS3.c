#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 100

/* Structure to store process information */
struct Process {
    int pid;
    int arrival;
    int burst;
};

/*
 * Scheduler function for PS3.c (STCF / Shortest Remaining Time First)
 * p[] : array of processes (pid, arrival, burst)
 * n   : number of processes
 */
void stcf(struct Process p[], int n) {
    int remaining[MAX];
    int completed = 0;
    int t = 0;

    /* Initialize remaining burst times */
    for (int i = 0; i < n; i++) {
        remaining[i] = p[i].burst;
    }

    while (completed < n) {
        int chosen = -1;

        /* Select available process with minimum remaining burst time;
           resolve ties using lowest pid */
        for (int i = 0; i < n; i++) {
            if (p[i].arrival <= t && remaining[i] > 0) {
                if (chosen == -1 ||
                    remaining[i] < remaining[chosen] ||
                    (remaining[i] == remaining[chosen] && p[i].pid < p[chosen].pid)) {
                    chosen = i;
                }
            }
        }

        /* If no process is ready at time t, CPU is idle */
        if (chosen == -1) {
            t++;
            continue;
        }

        /* Execute chosen process for 1 time unit */
        printf("Time %2d-%2d : Process %d\n", t, t + 1, p[chosen].pid);
        remaining[chosen]--;

        if (remaining[chosen] == 0) {
            completed++;
        }
        t++;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file.csv>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    struct Process p[MAX];
    int n = 0;
    char line[256];

    /* Read CSV file line by line */
    while (fgets(line, sizeof(line), fp)) {
        /* Skip empty lines */
        if (strlen(line) <= 1) continue;

        int pid, arrival, burst;
        /* Parse pid, arrival, burst (supporting comma or whitespace separated) */
        if (sscanf(line, "%d,%d,%d", &pid, &arrival, &burst) == 3 ||
            sscanf(line, "%d %d %d", &pid, &arrival, &burst) == 3) {
            p[n].pid = pid;
            p[n].arrival = arrival;
            p[n].burst = burst;
            n++;
        }
    }

    fclose(fp);

    if (n == 0) {
        fprintf(stderr, "No valid process records found in %s\n", argv[1]);
        return 1;
    }

    stcf(p, n);

    return 0;
}
