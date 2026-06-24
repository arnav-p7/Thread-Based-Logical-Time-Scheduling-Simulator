/*
SJF_threads.c - Thread-based SJF (non-preemptive)
UPGRADED WITH:
- IO Burst support
- results.txt export for charts
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    int pid;
    int arrival;
    int burst;
    int priority;
    int remaining;
    int completed;
    int start_time;
    int completion_time;
    int index;
    int exec_time;

    int io_burst;
    int doing_io;

} Process;

static Process *procs = NULL;
static int P = 0;
static pthread_mutex_t sched_mutex;
static pthread_cond_t sched_cond;
static int current_idx = -1;
static int scheduler_turn_done = 0;
static int processes_done = 0;

/* worker thread */
static void *worker(void *arg) {
    Process *p = (Process *)arg;

    while (1) {
        pthread_mutex_lock(&sched_mutex);

        while (current_idx != p->index && processes_done == 0)
            pthread_cond_wait(&sched_cond, &sched_mutex);

        if (processes_done) {
            pthread_mutex_unlock(&sched_mutex);
            break;
        }

        if (current_idx != p->index) {
            pthread_mutex_unlock(&sched_mutex);
            continue;
        }

        int s = p->start_time;
        int c = p->completion_time;
        int et = p->exec_time;

        pthread_mutex_unlock(&sched_mutex);

        printf("[SJF] Process %d ran from %d to %d (exec=%d)\n",
                p->pid, s, c, et);

        pthread_mutex_lock(&sched_mutex);
        scheduler_turn_done = 1;
        pthread_cond_signal(&sched_cond);
        pthread_mutex_unlock(&sched_mutex);
    }

    return NULL;
}

void run_SJF_from_main(int n, int *pid, int *arrival, int *burst,
                       int *priority, int *io_burst, int quantum)
{
    (void)quantum;
    printf("\n==== SJF (threads, logical-time, IO-enabled) ====\n");

    P = n;
    procs = malloc(sizeof(Process) * P);
    pthread_t *threads = malloc(sizeof(pthread_t) * P);

    for (int i = 0; i < P; i++) {
        procs[i].pid = pid[i];
        procs[i].arrival = arrival[i];
        procs[i].burst = burst[i];
        procs[i].priority = priority[i];
        procs[i].remaining = burst[i];
        procs[i].completed = 0;
        procs[i].start_time = -1;
        procs[i].completion_time = -1;
        procs[i].index = i;
        procs[i].exec_time = 0;
        procs[i].io_burst = io_burst[i];
        procs[i].doing_io = 0;
    }

    pthread_mutex_init(&sched_mutex, NULL);
    pthread_cond_init(&sched_cond, NULL);

    for (int i = 0; i < P; i++)
        pthread_create(&threads[i], NULL, worker, &procs[i]);

    int completed_count = 0;
    int current_time = 0;

    while (completed_count < P) {

        /* Update IO bursts */
        for (int i = 0; i < P; i++) {
            if (procs[i].doing_io) {
                procs[i].io_burst--;
                if (procs[i].io_burst <= 0) {
                    procs[i].doing_io = 0;
                    procs[i].arrival = current_time;
                }
            }
        }

        /* select shortest available burst */
        int idx = -1;
        int best = 1 << 30;

        for (int i = 0; i < P; i++) {
            if (!procs[i].completed &&
                !procs[i].doing_io &&
                procs[i].arrival <= current_time)
            {
                if (procs[i].burst < best) {
                    best = procs[i].burst;
                    idx = i;
                }
            }
        }

        if (idx == -1) {
            int next_arr = 1 << 30;
            for (int i = 0; i < P; i++) {
                if (!procs[i].completed &&
                    !procs[i].doing_io &&
                    procs[i].arrival < next_arr)
                {
                    next_arr = procs[i].arrival;
                }
            }
            current_time = (next_arr == (1 << 30)) ? current_time + 1 : next_arr;
            continue;
        }

        /* run selected job completely */
        int exec = procs[idx].remaining;
        int start = (current_time < procs[idx].arrival)
                    ? procs[idx].arrival : current_time;
        int completion = start + exec;

        pthread_mutex_lock(&sched_mutex);

        procs[idx].start_time = start;
        procs[idx].completion_time = completion;
        procs[idx].exec_time = exec;

        procs[idx].remaining = 0;
        procs[idx].completed = 1;

        scheduler_turn_done = 0;
        current_idx = idx;
        pthread_cond_broadcast(&sched_cond);

        while (!scheduler_turn_done)
            pthread_cond_wait(&sched_cond, &sched_mutex);

        current_idx = -1;
        pthread_mutex_unlock(&sched_mutex);

        if (procs[idx].io_burst > 0)
            procs[idx].doing_io = 1;

        current_time = completion;

        completed_count = 0;
        for (int i = 0; i < P; i++)
            if (procs[i].completed)
                completed_count++;
    }

    pthread_mutex_lock(&sched_mutex);
    processes_done = 1;
    pthread_cond_broadcast(&sched_cond);
    pthread_mutex_unlock(&sched_mutex);

    for (int i = 0; i < P; i++)
        pthread_join(threads[i], NULL);

    printf("\nSJF Summary:\nPID\tArrival\tBurst\tStart\tCompletion\tTurnaround\tWaiting\n");

    double total_wt = 0, total_tat = 0;

    for (int i = 0; i < P; i++) {
        int st = procs[i].start_time;
        int ct = procs[i].completion_time;
        int tat = ct - procs[i].arrival;
        int wt = tat - procs[i].burst;

        if (st < 0) st = 0;

        printf("%d\t%d\t%d\t%d\t%d\t\t%d\t\t%d\n",
               procs[i].pid,
               procs[i].arrival,
               procs[i].burst,
               st, ct, tat, wt);

        total_wt += wt;
        total_tat += tat;
    }

    printf("Average Waiting Time = %.2f\n", total_wt / P);
    printf("Average Turnaround Time = %.2f\n", total_tat / P);

    FILE *fp = fopen("results.txt", "a");
    if (fp) {
        fprintf(fp, "SJF,%.2f,%.2f\n", total_wt / P, total_tat / P);
        fclose(fp);
    }

    free(procs);
    free(threads);
    pthread_mutex_destroy(&sched_mutex);
    pthread_cond_destroy(&sched_cond);
}
