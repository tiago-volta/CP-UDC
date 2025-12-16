# Concurrency & Parallelism Projects

![C](https://img.shields.io/badge/Language-C-00599C?style=flat-square&logo=c&logoColor=white)
![Erlang](https://img.shields.io/badge/Language-Erlang-A90533?style=flat-square&logo=erlang&logoColor=white)
![MPI](https://img.shields.io/badge/Lib-OpenMPI-000000?style=flat-square&logo=linux&logoColor=white)
![Score](https://img.shields.io/badge/Score-100%25-success?style=flat-square)

## ⚡ Overview
This repository contains the complete coursework for the **Concurrency and Parallelism** subject at **Universidade da Coruña (FIC)**.

The project explores high-performance computing across two paradigms:
1.  **Shared Memory Concurrency:** Low-level synchronization using C/pthreads and Actor-based message passing with Erlang.
2.  **Distributed Parallelism:** HPC algorithms using C and MPI (Message Passing Interface) for cluster environments.

## 🏆 Performance Record
Achieved a **Perfect Score (100%)** across all 6 assignments, demonstrating mastery of thread safety, distributed algorithms, and optimization.

| Module | Project | Tech | Score | Status |
| :--- | :--- | :--- | :--- | :--- |
| **Concurrency** | **P1:** Race Conditions & Mutexes | C / pthreads | **0.50 / 0.50** | ✅ Perfect |
| | **P2:** Custom Sync Libs (Semaphores) | C / pthreads | **0.75 / 0.75** | ✅ Perfect |
| | **P3:** Actor Model & Message Passing | Erlang | **0.75 / 0.75** | ✅ Perfect |
| **Parallelism** | **P1:** Monte Carlo PI (SPMD) | C / MPI | **0.25 / 0.25** | ✅ Perfect |
| | **P2:** Custom MPI Collectives | C / MPI | **0.50 / 0.50** | ✅ Perfect |
| | **P3:** Mandelbrot Set Decomposition | C / MPI | **0.75 / 0.75** | ✅ Perfect |
| **Total** | **Final Assessment** | | **3.50 / 3.50** | **100%** |

## 🛠️ Technical Highlights

### 🧵 Part I: Concurrency (C & Erlang)
**P1: Variable Swapping System**
- Resolved race conditions in a shared array accessed by concurrent threads using granular locking strategies.
- Implemented global vs. fine-grained mutexes to benchmark performance and eliminate deadlocks.

**P2: Custom Synchronization Libraries**
- Engineered custom implementations of standard synchronization primitives from scratch:
    - **Recursive Mutexes:** Allowing re-entrant locking for complex thread logic.
    - **Read/Write Mutexes:** Optimizing for high-read/low-write scenarios.
    - **Semaphores:** Implemented using conditions/mutexes to solve the **Sleeping Barber Problem**.

**P3: Distributed Systems in Erlang**
- Implementation of fault-tolerant actor systems:
    - **Stack Module:** Functional stack implementation without stateful processes.
    - **Process Ring:** A scalable ring topology where messages circulate `N` times among nodes.

---

### 🚀 Part II: Parallelism (MPI)
**P1: Monte Carlo PI Estimation**
- Implemented the SPMD (Single Program Multiple Data) pattern to distribute stochastic simulations across nodes.
- Managed I/O via Rank 0 to prevent race conditions in output.

**P2: Advanced MPI Collectives**
- Optimization of P1 using custom collective communication algorithms:
    - **Binomial Tree Broadcast:** Implemented a log(N) complexity broadcast algorithm manually to outperform linear distribution.
    - **Flat-Tree Reduce:** Custom implementation of data gathering.

**P3: Mandelbrot Set (Domain Decomposition)**
- Parallel rendering of the Mandelbrot fractal using domain decomposition (row-based splitting).
- Optimized load balancing to handle the irregular computational cost of fractal points.
