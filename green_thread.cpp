/**
 * ============================================================
 *  GREEN THREAD OS SIMULATOR
 *  Demonstrates: User-level threads, Round-Robin scheduling,
 *                Context switching, Mutex locking
 * ============================================================
 *
 *  WHAT ARE GREEN THREADS?
 *  Green threads are user-space threads managed by a runtime
 *  (not the OS kernel). They are faster to create and switch
 *  than OS threads because no kernel call is needed.
 *
 *  KEY CONCEPTS SHOWN HERE:
 *  1. ucontext_t  → stores CPU register state (program counter,
 *                   stack pointer, etc.) for each thread
 *  2. makecontext → sets up a new execution context
 *  3. swapcontext → performs the actual context switch
 *  4. Round-Robin → each thread gets one "time slice" then yields
 *  5. Mutex       → prevents two threads entering critical section
 * ============================================================
 */

#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <ucontext.h>   // POSIX context switching API
#include <cstring>
#include <cassert>
#include <cstdlib>

// ── Configuration ────────────────────────────────────────────
// These are overridden at runtime via command-line arguments
static int NUM_THREADS  = 3;   // number of green threads to spawn
static int ITERATIONS   = 4;   // how many times each thread runs

// ── Thread States ─────────────────────────────────────────────
// A thread is always in exactly one of these states.
enum ThreadState {
    READY,    // waiting in the run queue, ready to execute
    RUNNING,  // currently executing on the CPU
    BLOCKED,  // waiting for a mutex / resource
    FINISHED  // execution complete
};

// Helper: state name → string (used in logs)
static const char* stateStr(ThreadState s) {
    switch (s) {
        case READY:    return "READY";
        case RUNNING:  return "RUNNING";
        case BLOCKED:  return "BLOCKED";
        case FINISHED: return "FINISHED";
    }
    return "UNKNOWN";
}

// ── Stack size for each green thread ─────────────────────────
static const int STACK_SIZE = 64 * 1024;   // 64 KB per thread

// ── Forward declarations ──────────────────────────────────────
struct GreenThread;
struct Mutex;
class Scheduler;

// ── Global Scheduler pointer (needed inside thread functions) ─
static Scheduler* g_sched = nullptr;

// ── Log helper: emits structured lines read by the frontend ───
//   Format:  [TYPE] message
//   Types:   SYSTEM | THREAD | MUTEX | SWITCH | SCHED
static void log(const std::string& type, const std::string& msg) {
    std::cout << "[" << type << "] " << msg << std::endl;
    std::cout.flush();
}

// ─────────────────────────────────────────────────────────────
//  GreenThread  –  represents one user-level thread
// ─────────────────────────────────────────────────────────────
struct GreenThread {
    int         id;             // unique thread identifier
    ThreadState state;          // current state
    ucontext_t  ctx;            // saved CPU context (registers + stack)
    char*       stack;          // thread's private stack
    int         iterations;     // how many more iterations to run
    std::string name;           // human-readable name for logs

    GreenThread(int id_, int iters, const std::string& n)
        : id(id_), state(READY), stack(nullptr),
          iterations(iters), name(n) {}

    ~GreenThread() { delete[] stack; }
};

// ─────────────────────────────────────────────────────────────
//  Mutex  –  a simple spinless green-thread mutex
//  Because we are single-threaded at the OS level, a "locked"
//  mutex just blocks the calling green thread and re-queues it
//  when the lock is released.
// ─────────────────────────────────────────────────────────────
struct Mutex {
    bool              locked;      // is anyone holding the lock?
    int               owner;       // thread id of lock holder (-1 = nobody)
    std::queue<int>   waitQueue;   // threads waiting for this mutex

    Mutex() : locked(false), owner(-1) {}
};

// ─────────────────────────────────────────────────────────────
//  Scheduler  –  Round-Robin green thread scheduler
// ─────────────────────────────────────────────────────────────
class Scheduler {
public:
    std::vector<GreenThread*> threads;   // all threads
    std::queue<int>           readyQ;    // ids of READY threads
    int                       current;   // id of RUNNING thread (-1 = none)
    ucontext_t                mainCtx;   // scheduler's own context
    Mutex                     sharedMutex; // one shared mutex for demo

    Scheduler() : current(-1) {}

    ~Scheduler() {
        for (auto* t : threads) delete t;
    }

    // ── Add a new thread to the scheduler ──────────────────
    void addThread(int iters, const std::string& name) {
        int id = (int)threads.size();
        auto* t = new GreenThread(id, iters, name);

        // Allocate a private stack for this thread
        t->stack = new char[STACK_SIZE];

        // getcontext snapshots the current execution environment
        // (signal mask, stack, registers) into t->ctx
        getcontext(&t->ctx);

        // Override the stack to use our allocated buffer
        t->ctx.uc_stack.ss_sp   = t->stack;
        t->ctx.uc_stack.ss_size = STACK_SIZE;

        // When this context finishes, resume the scheduler context
        t->ctx.uc_link = &mainCtx;

        // makecontext sets the function that will run when this
        // context is first swapped into.
        // We pass the thread id so it knows which thread it is.
        makecontext(&t->ctx, (void(*)())threadEntry, 1, id);

        threads.push_back(t);
        readyQ.push(id);

        log("SYSTEM", "Thread-" + std::to_string(id) + " (" + name
            + ") created → STATE:READY");
    }

    // ── Main scheduling loop ────────────────────────────────
    void run() {
        log("SCHED", "Scheduler started. Threads: "
            + std::to_string(threads.size())
            + "  Algorithm: Round-Robin");

        while (!readyQ.empty()) {
            // Pick next thread from front of ready queue (Round-Robin)
            int nextId = readyQ.front();
            readyQ.pop();

            GreenThread* next = threads[nextId];
            if (next->state == FINISHED) continue;  // skip done threads

            // Transition: READY → RUNNING
            next->state = RUNNING;
            current = nextId;

            log("SWITCH", "Context switch → Thread-"
                + std::to_string(nextId) + " (" + next->name + ")"
                + "  STATE:" + stateStr(next->state));

            /*
             *  swapcontext(from, to)
             *  ─────────────────────
             *  1. Saves all CPU registers into *from  (mainCtx here)
             *  2. Loads  CPU registers from   *to    (thread context)
             *  3. Execution continues inside the thread
             *
             *  When the thread calls yield() it does the reverse:
             *  swapcontext(thread_ctx, mainCtx)
             */
            swapcontext(&mainCtx, &next->ctx);

            // ── We land here after thread yields / finishes ──
            current = -1;
        }

        log("SCHED", "All threads finished. Simulation complete.");
        printSummary();
    }

    // ── Called by a thread to voluntarily yield the CPU ────
    void yield(int threadId) {
        GreenThread* t = threads[threadId];

        if (t->iterations > 0) {
            // Thread still has work to do → put back in ready queue
            t->state = READY;
            readyQ.push(threadId);
            log("SCHED", "Thread-" + std::to_string(threadId)
                + " yielded → STATE:READY  remaining_iters:"
                + std::to_string(t->iterations));
        } else {
            // Thread is done
            t->state = FINISHED;
            log("THREAD", "Thread-" + std::to_string(threadId)
                + " (" + t->name + ") FINISHED");
        }

        // Swap back to scheduler context
        swapcontext(&t->ctx, &mainCtx);
    }

    // ── Mutex lock (blocks if already locked) ──────────────
    void mutexLock(int threadId) {
        if (!sharedMutex.locked) {
            // Lock is free → acquire immediately
            sharedMutex.locked = true;
            sharedMutex.owner  = threadId;
            log("MUTEX", "Thread-" + std::to_string(threadId)
                + " ACQUIRED mutex");
        } else {
            // Lock is held by another thread → BLOCK this thread
            GreenThread* t = threads[threadId];
            t->state = BLOCKED;
            sharedMutex.waitQueue.push(threadId);

            log("MUTEX", "Thread-" + std::to_string(threadId)
                + " BLOCKED on mutex (held by Thread-"
                + std::to_string(sharedMutex.owner) + ")  STATE:BLOCKED");

            // Yield CPU back to scheduler without re-queuing
            swapcontext(&t->ctx, &mainCtx);
        }
    }

    // ── Mutex unlock (wakes up next waiter if any) ─────────
    void mutexUnlock(int threadId) {
        assert(sharedMutex.owner == threadId);

        log("MUTEX", "Thread-" + std::to_string(threadId)
            + " RELEASED mutex");

        if (!sharedMutex.waitQueue.empty()) {
            // Wake up next blocked thread
            int nextId = sharedMutex.waitQueue.front();
            sharedMutex.waitQueue.pop();

            sharedMutex.owner = nextId;
            GreenThread* next = threads[nextId];
            next->state = READY;
            readyQ.push(nextId);

            log("MUTEX", "Thread-" + std::to_string(nextId)
                + " UNBLOCKED (acquired mutex)  STATE:READY");
        } else {
            // No waiters → release lock completely
            sharedMutex.locked = false;
            sharedMutex.owner  = -1;
        }
    }

    // ── Print final summary ─────────────────────────────────
    void printSummary() {
        log("SYSTEM", "=== THREAD SUMMARY ===");
        for (auto* t : threads) {
            log("SYSTEM", "Thread-" + std::to_string(t->id)
                + " (" + t->name + ")  final_state:"
                + stateStr(t->state));
        }
    }

    // ── Static entry point for all threads ─────────────────
    //    makecontext requires a C-style function pointer
    static void threadEntry(int threadId) {
        g_sched->threadMain(threadId);
    }

    // ── The actual work each thread performs ───────────────
    void threadMain(int threadId) {
        GreenThread* t = threads[threadId];

        log("THREAD", "Thread-" + std::to_string(threadId)
            + " (" + t->name + ") started  STATE:RUNNING");

        while (t->iterations > 0) {
            t->iterations--;

            log("THREAD", "Thread-" + std::to_string(threadId)
                + " (" + t->name + ") iter_left:"
                + std::to_string(t->iterations)
                + "  STATE:RUNNING");

            // Every other iteration, demonstrate mutex usage
            if (t->iterations % 2 == 0) {
                mutexLock(threadId);

                // ── Critical section ──────────────────────
                // Only one thread can be here at a time
                log("THREAD", "Thread-" + std::to_string(threadId)
                    + " inside CRITICAL SECTION");
                // (simulate a tiny bit of work)

                mutexUnlock(threadId);
            }

            // Yield CPU to scheduler (cooperative multitasking)
            yield(threadId);
        }

        // Thread function returns → uc_link restores mainCtx
    }
};

// ─────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    // Parse optional command-line args: ./green_thread <threads> <iters>
    if (argc >= 2) NUM_THREADS  = std::atoi(argv[1]);
    if (argc >= 3) ITERATIONS   = std::atoi(argv[2]);

    // Clamp to sane values
    if (NUM_THREADS < 1) NUM_THREADS = 1;
    if (NUM_THREADS > 8) NUM_THREADS = 8;
    if (ITERATIONS  < 1) ITERATIONS  = 1;
    if (ITERATIONS  > 10) ITERATIONS = 10;

    log("SYSTEM", "Green Thread OS Simulator starting");
    log("SYSTEM", "Config → threads:" + std::to_string(NUM_THREADS)
        + "  iterations:" + std::to_string(ITERATIONS));

    // Build the scheduler
    Scheduler sched;
    g_sched = &sched;

    // Thread name pool for readable logs
    const std::string names[] = {
        "Alpha", "Beta", "Gamma", "Delta",
        "Epsilon", "Zeta", "Eta", "Theta"
    };

    // Create all threads
    for (int i = 0; i < NUM_THREADS; i++) {
        sched.addThread(ITERATIONS, names[i % 8]);
    }

    // Run the Round-Robin scheduler
    sched.run();

    return 0;
}
