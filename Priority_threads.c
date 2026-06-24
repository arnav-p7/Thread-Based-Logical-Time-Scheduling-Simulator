/*
Priority_threads.c - Thread-based Priority scheduling (logical time, PREEMPTIVE)
UPGRADED WITH:
- Preemptive priority (runs in logical 1-unit quanta so higher-priority arrivals preempt)
- IO burst simulation (io_burst, doing_io)
- results.txt export for charting
Lower numeric 'priority' value => higher priority.
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

    /* NEW: I/O fields */
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

/* worker thread: waits until scheduler selects this process, then prints */
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
        int rem = p->remaining;

        pthread_mutex_unlock(&sched_mutex);

        printf("[Priority] Process %d ran from %d to %d (exec=%d, remaining after=%d)\n",
               p->pid, s, c, et, rem);

        pthread_mutex_lock(&sched_mutex);
        scheduler_turn_done = 1;
        pthread_cond_signal(&sched_cond);
        pthread_mutex_unlock(&sched_mutex);
    }
    return NULL;
}

/*
  UPDATED function signature: added io_burst array (to support IO simulation).
  quantum parameter is accepted but not used for priority scheduling here.
*/
void run_Priority_from_main(int n, int *pid, int *arrival, int *burst, int *priority, int *io_burst, int quantum) {
    printf("\n==== Priority (threads, logical-time, PREEMPTIVE, IO-enabled) ====\n");

    P = n;
    procs = malloc(sizeof(Process) * P);
    pthread_t *threads = malloc(sizeof(pthread_t) * P);

    /* initialize processes (including new io fields) */
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

        /* NEW */
        procs[i].io_burst = io_burst ? io_burst[i] : 0;
        procs[i].doing_io = 0;

        /* If burst is zero, treat as immediately completable when scheduled.
           We keep completed=0 here; scheduler will handle zero-burst items at arrival time. */
    }

    pthread_mutex_init(&sched_mutex, NULL);
    pthread_cond_init(&sched_cond, NULL);

    for (int i = 0; i < P; i++)
        pthread_create(&threads[i], NULL, worker, &procs[i]);

    int completed_count = 0;
    int current_time = 0;

    while (completed_count < P) {
        /* 1) progress I/O for processes in IO */
        for (int i = 0; i < P; i++) {
            if (procs[i].doing_io) {
                procs[i].io_burst--;
                if (procs[i].io_burst <= 0) {
                    procs[i].doing_io = 0;
                    procs[i].arrival = current_time; /* returns to ready queue at current time */
                }
            }
        }

        /* 2) pick highest priority ready process (lowest numeric value),
              skipping processes that are completed or doing IO.
              If a process has remaining==0 but not marked completed (edge-case), mark it completed.
         */
        int idx = -1;
        int best = 1 << 30;
        for (int i = 0; i < P; i++) {
            if (procs[i].completed || procs[i].doing_io) continue;

            if (procs[i].arrival <= current_time) {
                if (procs[i].remaining <= 0) {
                    /* zero-length process: mark completed instantly at its arrival (if not already) */
                    procs[i].completed = 1;
                    procs[i].start_time = procs[i].arrival > current_time ? procs[i].arrival : current_time;
                    procs[i].completion_time = procs[i].start_time;
                    continue;
                }
                if (procs[i].priority < best) {
                    best = procs[i].priority;
                    idx = i;
                }
            }
        }

        if (idx == -1) {
            /* No ready process now — jump to next arrival (or advance time by 1) */
            int next_arr = 1 << 30;
            for (int i = 0; i < P; i++) {
                if (procs[i].completed || procs[i].doing_io) continue;
                if (procs[i].arrival < next_arr) next_arr = procs[i].arrival;
            }
            if (next_arr == (1 << 30)) {
                /* nothing left? break (safety) */
                break;
            }
            /* jump time forward to next event (arrival or wakeup from IO) */
            current_time = (next_arr > current_time) ? next_arr : current_time + 1;
            continue;
        }

        /* 3) Run selected process for ONE logical time unit (preemptive) */
        int exec = (procs[idx].remaining > 0) ? 1 : 0;
        int start = (current_time < procs[idx].arrival) ? procs[idx].arrival : current_time;
        int completion = start + exec;

        /* Set start_time only if first time it runs */
        if (procs[idx].start_time < 0) procs[idx].start_time = start;

        pthread_mutex_lock(&sched_mutex);

        procs[idx].start_time = procs[idx].start_time; /* already set above */
        procs[idx].completion_time = completion;
        procs[idx].exec_time = exec;

        procs[idx].remaining -= exec;
        if (procs[idx].remaining <= 0) {
            procs[idx].remaining = 0;
            procs[idx].completed = 1;
            procs[idx].completion_time = completion;
        }

        scheduler_turn_done = 0;
        current_idx = idx;
        pthread_cond_broadcast(&sched_cond);

        while (!scheduler_turn_done)
            pthread_cond_wait(&sched_cond, &sched_mutex);

        current_idx = -1;
        pthread_mutex_unlock(&sched_mutex);

        /* 4) after running 1 unit, if process has IO configured and not completed, put it to IO */
        if (procs[idx].io_burst > 0 && !procs[idx].completed) {
            procs[idx].doing_io = 1;
            /* Note: io_burst will be decremented in the IO progress block above in subsequent loops */
        }

        current_time = completion;

        /* 5) recompute completed_count */
        completed_count = 0;
        for (int i = 0; i < P; i++) if (procs[i].completed) completed_count++;
    }

    /* signal workers to exit */
    pthread_mutex_lock(&sched_mutex);
    processes_done = 1;
    pthread_cond_broadcast(&sched_cond);
    pthread_mutex_unlock(&sched_mutex);

    for (int i = 0; i < P; i++) pthread_join(threads[i], NULL);

    /* Print summary and write results for charts */
    printf("\n\nPriority Summary:\nPID\tArrival\tBurst\tPriority\tStart\tCompletion\tTurnaround\tWaiting\n");
    double total_wt = 0, total_tat = 0;
    for (int i = 0; i < P; i++) {
        int st = procs[i].start_time;
        int ct = procs[i].completion_time;
        if (st < 0) st = 0;
        if (ct < 0) ct = st; /* safety */
        int tat = ct - procs[i].arrival;
        int wt = tat - procs[i].burst;
        printf("%d\t%d\t%d\t%d\t\t%d\t%d\t\t%d\t\t%d\n",
               procs[i].pid, procs[i].arrival, procs[i].burst, procs[i].priority,
               st, ct, tat, wt);
        total_wt += wt;
        total_tat += tat;
    }
    printf("Average Waiting Time = %.2f\n", total_wt / P);
    printf("Average Turnaround Time = %.2f\n", total_tat / P);

    /* append to results.txt for charting */
    FILE *fp = fopen("results.txt", "a");
    if (fp) {
        fprintf(fp, "Priority,%.2f,%.2f\n", total_wt / P, total_tat / P);
        fclose(fp);
    }

    free(procs);
    free(threads);
    pthread_mutex_destroy(&sched_mutex);
    pthread_cond_destroy(&sched_cond);
}
