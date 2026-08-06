# Module 12 — Systems Programming

Where C stops being a language and starts being the interface to the operating system.
Everything here is **POSIX**, not ISO C — it works on Linux, macOS and BSD, and needs a
compatibility layer on Windows.

---

## 1. System calls vs library functions

```
your code  →  printf()      ← libc, buffered, portable ISO C
                  ↓
              write(2)       ← SYSTEM CALL: a trap into the kernel
```

A system call costs ~100–500ns (a mode switch, cache and TLB effects). A libc function that
buffers — `printf`, `fwrite` — batches many logical writes into one syscall. That is why
`printf` in a loop is far faster than `write` in a loop, and why `strace` on a well-written
program shows fewer syscalls than you expect.

| Layer | Example | Buffered | Portable |
|---|---|---|---|
| ISO C stdio | `fopen`, `fprintf`, `fread` | yes | everywhere |
| POSIX I/O | `open`, `write`, `read` | **no** | POSIX |
| Raw syscall | `syscall(SYS_write, ...)` | no | Linux-specific |

File descriptors are small integers: 0 = stdin, 1 = stdout, 2 = stderr.

---

## 2. Processes

```c
pid_t pid = fork();          /* returns TWICE: 0 in the child, child's pid in the parent */
if (pid == 0) {
    execvp("ls", args);      /* REPLACES this process image; never returns on success */
    _exit(127);              /* only reached if exec failed */
} else if (pid > 0) {
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) printf("exit code %d\n", WEXITSTATUS(status));
}
```

- **`fork` returns twice.** That is the whole trick and the whole confusion.
- The child gets a **copy** of the parent's memory — copy-on-write, so it is cheap until
  one of them writes.
- **`exec` does not return.** It replaces the program running in this process.
- If the parent does not `wait`, the finished child becomes a **zombie** (a process table
  entry holding an exit status nobody collected). If the parent dies first, the child is
  **orphaned** and reparented to `init`.
- In the child, use `_exit` rather than `exit` — `exit` runs `atexit` handlers and flushes
  stdio buffers that the parent also holds, producing duplicated output.

---

## 3. Pipes and redirection

```c
int fd[2];
pipe(fd);                    /* fd[0] = read end, fd[1] = write end */
```

A pipe is a unidirectional in-kernel byte buffer (64 KB on Linux). **Close the ends you do
not use** — a reader only sees EOF when *every* copy of the write end is closed, so a
forgotten descriptor is a hang that looks like a deadlock.

`dup2(fd, STDOUT_FILENO)` is how a shell wires a command's stdout into the next command's
stdin.

---

## 4. Signals

Asynchronous notifications: `SIGINT` (Ctrl-C), `SIGTERM`, `SIGSEGV`, `SIGCHLD`, `SIGPIPE`.

**A signal handler runs at an arbitrary point in your program.** The only things you may
safely do inside one:

- call an **async-signal-safe** function (`write`, `_exit`, `signal` — *not* `printf`,
  *not* `malloc`),
- assign to a `volatile sig_atomic_t` flag.

The correct pattern is: set a flag in the handler, act on it in the main loop.

Use `sigaction` rather than `signal` — `signal`'s semantics vary between systems.

---

## 5. Threads

```c
pthread_t t;
pthread_create(&t, NULL, worker, arg);
pthread_join(t, &result);
```

Threads share **everything**: heap, globals, file descriptors. Only the stack and registers
are per-thread. That makes communication free and correctness hard.

| Primitive | Use |
|---|---|
| `pthread_mutex_t` | mutual exclusion around shared state |
| `pthread_cond_t` | wait for a condition; **always** paired with a mutex |
| `pthread_rwlock_t` | many readers or one writer |
| `sem_t` | counting semaphore |
| `<stdatomic.h>` | lock-free counters and flags |

**A data race is undefined behaviour**, not "a wrong answer sometimes". Two threads
accessing the same object, at least one writing, with no synchronisation, means the whole
program is meaningless. `-fsanitize=thread` finds these.

`pthread_cond_wait` must **always** be called in a `while` loop, never an `if`, because of
spurious wakeups and because another thread may consume the condition before you wake.

**False sharing**: two threads writing to different variables that share a 64-byte cache
line will invalidate each other's caches on every write. `alignas(64)` fixes it, and the
difference can be 10×.

---

## 6. Sockets

```c
int fd = socket(AF_INET, SOCK_STREAM, 0);   /* TCP */
bind(fd, ...); listen(fd, backlog);
int client = accept(fd, ...);               /* blocks until a connection arrives */
```

`SO_REUSEADDR` avoids "Address already in use" during the TIME_WAIT period after a restart.
**TCP is a byte stream, not a message stream**: one `write` may arrive as three `read`s, or
three writes as one read. You must frame your own messages (length prefix, or a delimiter).

---

## Practice

| File | Contents |
|---|---|
| `practice/01_processes.c` | `fork`/`exec`/`wait`, exit statuses, zombies, orphans, copy-on-write |
| `practice/02_pipes.c` | Pipes, `dup2` redirection, a two-stage pipeline like the shell builds |
| `practice/03_signals.c` | `sigaction`, the flag pattern, async-signal-safety, `SIGCHLD` reaping |
| `practice/04_threads.c` | `pthread_create`/`join`, a data race demonstrated, mutex, condvar, a thread pool |
| `practice/05_mmap.c` | Memory-mapped files, shared anonymous memory between processes |

```bash
cd 12-Systems-Programming
gcc -std=c17 -Wall -Wextra -g practice/04_threads.c -o threads -lpthread && ./threads

# find the data race:
gcc -std=c17 -g -fsanitize=thread practice/04_threads.c -o threads_tsan -lpthread && ./threads_tsan
```

---

## Checklist

- [ ] You know `fork` returns twice and what each value means.
- [ ] You call `_exit` in a forked child, not `exit`.
- [ ] You close the unused ends of every pipe.
- [ ] You only set a `volatile sig_atomic_t` flag inside a signal handler.
- [ ] You wrap `pthread_cond_wait` in a `while`, never an `if`.
- [ ] You know a data race is undefined behaviour, not a rare wrong answer.
- [ ] You frame your own messages over TCP.
