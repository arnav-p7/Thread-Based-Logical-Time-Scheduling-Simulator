/*
main.c - Controller for Thread-Based Logical-Time Scheduling Simulator
Upgrades (driver-level):
- Clears results.txt at startup (fresh runs)
- Prints a banner telling user which features are enabled
- Invokes chart_generator.py after each algorithm run (hook for graph)
Notes:
- Per-algorithm files append results to results.txt for charting.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void find_best_algorithm()

{
    FILE *fp = fopen("results.txt", "r");
    if (!fp) {
        printf("\n[ERROR] Could not open results.txt to determine the best algorithm.\n");
        return;
    }

    char algo[50];
    double wt, tat;

    char best_algo_wt[50] = "";
    char best_algo_tat[50] = "";

    double best_wt = 1e9;   // very large number
    double best_tat = 1e9;

    printf("\n=== Comparing Algorithms (from results.txt) ===\n");

    while (fscanf(fp, "%[^,],%lf,%lf\n", algo, &wt, &tat) == 3) {
        printf("%s -> Waiting: %.2f, Turnaround: %.2f\n", algo, wt, tat);

        if (wt < best_wt) {
            best_wt = wt;
            strcpy(best_algo_wt, algo);
        }

        if (tat < best_tat) {
            best_tat = tat;
            strcpy(best_algo_tat, algo);
        }
    }

    fclose(fp);

    printf("\n============ BEST ALGORITHMS ============\n");
    printf("Best Avg Waiting Time     : %s (%.2f)\n", best_algo_wt, best_wt);
    printf("Best Avg Turnaround Time  : %s (%.2f)\n", best_algo_tat, best_tat);
    printf("=========================================\n\n");
}



/* Declarations of algorithm entry functions (implemented in separate files)
   Note: each function accepts an extra int* io_burst array.
*/
void run_FCFS_from_main(int n, int *pid, int *arrival, int *burst, int *priority, int *io_burst, int quantum);
void run_SJF_from_main(int n, int *pid, int *arrival, int *burst, int *priority, int *io_burst, int quantum);
void run_Priority_from_main(int n, int *pid, int *arrival, int *burst, int *priority, int *io_burst, int quantum);
void run_RR_from_main(int n, int *pid, int *arrival, int *burst, int *priority, int *io_burst, int quantum);

/* Try to read input from filename; return 1 on success
   Expected input format per process line:
     PID Arrival Burst Priority IO_Burst
   Final line: quantum (optional)
*/
int read_input_file(const char *filename, int *n, int **pid, int **arrival, int **burst, int **priority, int **io_burst, int *quantum) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;
    if (fscanf(fp, "%d", n) != 1) { fclose(fp); return 0; }

    *pid = malloc(sizeof(int) * (*n));
    *arrival = malloc(sizeof(int) * (*n));
    *burst = malloc(sizeof(int) * (*n));
    *priority = malloc(sizeof(int) * (*n));
    *io_burst = malloc(sizeof(int) * (*n));

    if (!*pid || !*arrival || !*burst || !*priority || !*io_burst) {
        /* allocation failure: clean up and fail */
        free(*pid); free(*arrival); free(*burst); free(*priority); free(*io_burst);
        fclose(fp);
        return 0;
    }

    for (int i = 0; i < *n; i++) {
        /* read 5 values now: PID Arrival Burst Priority IO_Burst */
        if (fscanf(fp, "%d %d %d %d %d", &(*pid)[i], &(*arrival)[i], &(*burst)[i], &(*priority)[i], &(*io_burst)[i]) != 5) {
            fclose(fp);
            return 0;
        }
    }
    if (fscanf(fp, "%d", quantum) != 1) *quantum = 2;
    fclose(fp);
    return 1;
}

/* Prompt the user for interactive input (now includes IO_Burst) */
int prompt_user_input(int *n, int **pid, int **arrival, int **burst, int **priority, int **io_burst, int *quantum) {
    printf("Enter number of processes: ");
    if (scanf("%d", n) != 1) return 0;
    *pid = malloc(sizeof(int) * (*n));
    *arrival = malloc(sizeof(int) * (*n));
    *burst = malloc(sizeof(int) * (*n));
    *priority = malloc(sizeof(int) * (*n));
    *io_burst = malloc(sizeof(int) * (*n));

    if (!*pid || !*arrival || !*burst || !*priority || !*io_burst) {
        free(*pid); free(*arrival); free(*burst); free(*priority); free(*io_burst);
        return 0;
    }

    for (int i = 0; i < *n; i++) {
        printf("Enter PID Arrival Burst Priority IO_Burst (space separated) for process %d: ", i+1);
        if (scanf("%d %d %d %d %d", &(*pid)[i], &(*arrival)[i], &(*burst)[i], &(*priority)[i], &(*io_burst)[i]) != 5) {
            return 0;
        }
    }
    printf("Enter time quantum for Round Robin (default 2): ");
    if (scanf("%d", quantum) != 1) *quantum = 2;
    return 1;
}

int main() {
    int n = 0, quantum = 2;
    int *pid = NULL, *arrival = NULL, *burst = NULL, *priority = NULL, *io_burst = NULL;
    int input_from_file = 0;

    /* ------------------ Prepare result file ------------------ */
    /* remove old results so charts start fresh (chart script depends on results.txt) */
    remove("results.txt");

    printf("\n=== Thread-Based Scheduling Simulator ===\n");
    printf("Features enabled: \n");
    printf(" - PREEMPTIVE Priority (if selected)\n");
    printf(" - I/O Burst Simulation\n");
    printf(" - Results export for charting (results.txt)\n\n");

    input_from_file = read_input_file("input.txt", &n, &pid, &arrival, &burst, &priority, &io_burst, &quantum);

    if (!input_from_file) {
        printf("No valid input.txt found. Switching to interactive mode.\n");
        if (!prompt_user_input(&n, &pid, &arrival, &burst, &priority, &io_burst, &quantum)) {
            fprintf(stderr, "Invalid input. Exiting.\n");
            /* free any partially allocated arrays */
            free(pid); free(arrival); free(burst); free(priority); free(io_burst);
            return 1;
        }
    } else {
        printf("Loaded input.txt (%d processes, quantum=%d).\n", n, quantum);
    }

    int choice = 0;
    if (!input_from_file) {
        printf("\nSelect scheduling option:\n");
        printf("1. FCFS\n2. SJF\n3. Priority\n4. Round Robin\n5. Run all sequentially\n");
        printf("Enter choice: ");
        if (scanf("%d", &choice) != 1) {
            free(pid); free(arrival); free(burst); free(priority); free(io_burst);
            return 1;
        }
    } else {
        /* If file provided, allow user to override or default to run all */
        printf("\nInput provided by file. Choose option to run (1-5). Press Enter for default (5 = all): ");
        char buf[16];
        if (fgets(buf, sizeof(buf), stdin) && buf[0] != '\n') {
            if (sscanf(buf, "%d", &choice) != 1) choice = 5;
        } else {
            choice = 5;
        }
    }

    switch (choice) {
        case 1:
            run_FCFS_from_main(n, pid, arrival, burst, priority, io_burst, quantum);
            /* create/update chart after run (hook: Improvement 4) */
            system("python3 chart_generator.py");
            break;
        case 2:
            run_SJF_from_main(n, pid, arrival, burst, priority, io_burst, quantum);
            system("python3 chart_generator.py");
            break;
        case 3:
            run_Priority_from_main(n, pid, arrival, burst, priority, io_burst, quantum);
            system("python3 chart_generator.py");
            break;
        case 4:
            run_RR_from_main(n, pid, arrival, burst, priority, io_burst, quantum);
            system("python3 chart_generator.py");
            break;
              case 5:
default:
    run_FCFS_from_main(n, pid, arrival, burst, priority, io_burst, quantum);
    system("python3 chart_generator.py");

    run_SJF_from_main(n, pid, arrival, burst, priority, io_burst, quantum);
    system("python3 chart_generator.py");

    run_Priority_from_main(n, pid, arrival, burst, priority, io_burst, quantum);
    system("python3 chart_generator.py");

    run_RR_from_main(n, pid, arrival, burst, priority, io_burst, quantum);
    system("python3 chart_generator.py");

    find_best_algorithm();

    break;

    }

    free(pid); free(arrival); free(burst); free(priority); free(io_burst);
    return 0;
}
