# 🧵 Thread-Based Logical-Time Scheduling Simulator

A CPU scheduling simulator built in C that runs each process as a real
POSIX thread, coordinated by a central scheduler using mutex locks and
condition variables — simulating logical-time execution of 4 classic
scheduling algorithms with I/O burst support and automated performance
visualization.

Built as part of the **Computer System Programming** course at
**Dhirubhai Ambani University of Technology (DA-IICT)**.

---

## ⚙️ Algorithms Implemented

| Algorithm | Type | Details |
|---|---|---|
| FCFS | Non-preemptive | First Come First Served |
| SJF | Non-preemptive | Shortest Job First |
| Round Robin | Preemptive | Configurable time quantum |
| Priority | Preemptive | Lower value = higher priority |

---

## 🧵 Threading Architecture

Each process runs as a dedicated **POSIX thread** (`pthread`).
A central scheduler coordinates execution using:

- `pthread_mutex_t` — mutual exclusion for shared state
- `pthread_cond_t` — condition variables for signaling between
  scheduler and worker threads
- Logical-time simulation — scheduler advances a global clock
  and dispatches threads one unit at a time (preemptive) or
  per full burst (non-preemptive)

---

## 💡 Features

- **I/O Burst Simulation** — processes can block for I/O mid-execution
  and re-enter the ready queue after their I/O burst completes
- **Preemptive Priority** — higher-priority arrivals preempt the
  currently running process at each logical time unit
- **Algorithm Comparison** — after all algorithms run, the simulator
  reads `results.txt` and prints the best algorithm by average
  waiting time and turnaround time
- **File & Interactive Input** — accepts process data via `input.txt`
  or interactive CLI prompts
- **Makefile** — single `make` command builds the entire project

---

## 📊 Performance Visualization

A Python script (`chart_generator.py`) auto-generates a side-by-side
bar chart comparing all algorithms on:

- Average Waiting Time
- Average Turnaround Time

Output saved as `results_chart.png` after each run.

---

## 🛠️ Tech Stack

- **Language**: C (C11)
- **Threading**: POSIX Threads (`pthread`)
- **Visualization**: Python 3, Matplotlib, NumPy
- **Build**: Makefile

---

## 🚀 How to Run

```bash
# Build
make

# Run
./main

# View chart
open results_chart.png    # macOS
xdg-open results_chart.png  # Linux
```

**Input format** (`input.txt`):

<number_of_processes>
  
<PID><Arrival><Burst><Priority><IO_Burst>

...

<quantum>

**Example:**

4

1 0 8 2 2

2 1 4 1 0

3 2 9 3 1

4 3 5 2 0

2
