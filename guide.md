# OS Lab Exam Quick Guide (Linux / C)

Processes · Scheduling · Signals · Pipes/FIFOs · Message Queues · Shared Memory · Semaphores · Threads · Sockets · Linux commands

---

## 0. Compile and Run

```bash
gcc -Wall -pthread prog.c -o prog            # threads, POSIX semaphores
gcc -Wall -pthread prog.c -o prog -lrt       # POSIX shm / mqueue (needed on older glibc, harmless on new)
./prog
echo $?                                      # exit status of last command
strace -f ./prog                             # trace syscalls (handy for debugging blocking)
```

System V IPC needs no extra flags. Always check return values and use `perror()`.

---

## 1. Header Files Cheat Sheet

| Topic | Headers | Key functions |
|---|---|---|
| Basics | `<stdio.h> <stdlib.h> <string.h> <errno.h>` | printf, exit, strlen, perror, errno |
| Process | `<unistd.h> <sys/types.h>` | fork, execl/execv/execlp, getpid, getppid, sleep, _exit |
| Wait | `<sys/wait.h>` | wait, waitpid, WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG |
| Signals | `<signal.h>` | kill, signal, sigaction, raise, pause, alarm |
| Scheduling | `<sched.h>` | sched_yield, sched_setscheduler, sched_getscheduler, `struct sched_param` |
| Pipe | `<unistd.h>` | pipe, read, write, close |
| FIFO | `<sys/stat.h> <fcntl.h> <unistd.h>` | mkfifo, open, read, write, unlink |
| POSIX shm | `<sys/mman.h> <fcntl.h> <sys/stat.h> <unistd.h>` | shm_open, ftruncate, mmap, munmap, shm_unlink |
| POSIX msg queue | `<mqueue.h> <fcntl.h> <sys/stat.h>` | mq_open, mq_send, mq_receive, mq_close, mq_unlink |
| POSIX semaphore | `<semaphore.h> <fcntl.h>` | sem_init, sem_open, sem_wait, sem_post, sem_destroy, sem_close, sem_unlink |
| System V (all) | `<sys/types.h> <sys/ipc.h>` | ftok, IPC_CREAT, IPC_RMID, IPC_PRIVATE |
| SysV shm | `<sys/shm.h>` | shmget, shmat, shmdt, shmctl |
| SysV msg queue | `<sys/msg.h>` | msgget, msgsnd, msgrcv, msgctl |
| SysV semaphore | `<sys/sem.h>` | semget, semop, semctl, `struct sembuf` |
| Threads | `<pthread.h>` | pthread_create/join/exit, mutex, cond, rwlock |
| Sockets | `<sys/socket.h> <netinet/in.h> <arpa/inet.h> <sys/un.h> <netdb.h>` | socket, bind, listen, accept, connect, send, recv |

---

## 2. POSIX vs System V at a Glance

| | POSIX | System V |
|---|---|---|
| Identified by | name string `"/name"` | integer key from `ftok()` (or `IPC_PRIVATE`) |
| Shared memory | `shm_open → ftruncate → mmap → munmap → shm_unlink` | `shmget → shmat → shmdt → shmctl(IPC_RMID)` |
| Message queue | `mq_open → mq_send / mq_receive → mq_close → mq_unlink` | `msgget → msgsnd / msgrcv → msgctl(IPC_RMID)` |
| Semaphore | `sem_open` (named) or `sem_init` (unnamed) `→ sem_wait / sem_post → sem_close/sem_destroy → sem_unlink` | `semget → semctl(SETVAL) → semop → semctl(IPC_RMID)` |
| Link flag | `-lrt -pthread` | none |
| Inspect | `ls /dev/shm`, `ls /dev/mqueue` | `ipcs`, `ipcrm` |
| Cleanup | `*_unlink` | `ipcrm` or `*ctl(IPC_RMID)` |

---

## 3. Processes

### Flow
```
fork() ──► child  (pid == 0): do work / execl() ... / exit()
      └──► parent (pid  > 0): waitpid(child) → inspect status
```

### Code
```c
#include <unistd.h>
#include <sys/wait.h>

pid_t pid = fork();
if (pid < 0) {
    perror("fork");
} else if (pid == 0) {                          /* CHILD */
    printf("child  pid=%d ppid=%d\n", getpid(), getppid());
    execl("/bin/ls", "ls", "-l", (char *)NULL); /* replaces the process image; last arg NULL */
    perror("execl");                            /* only reached if exec failed */
    _exit(1);
} else {                                        /* PARENT */
    int status;
    waitpid(pid, &status, 0);                   /* wait(&status) = wait for any child */
    if (WIFEXITED(status))
        printf("exited, code=%d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("killed by signal %d\n", WTERMSIG(status));
}
```

### Creating N children (the classic loop)
```c
for (int i = 0; i < 3; i++) {
    pid_t p = fork();
    if (p == 0) {            /* child MUST exit, or it keeps looping and forks too */
        /* ... work ... */
        exit(i);
    }
    pids[i] = p;             /* parent stores pid and continues the loop */
}
for (int i = 0; i < 3; i++) waitpid(pids[i], NULL, 0);   /* reap all: no zombies */
```

### Notes
- `exec` variants: `execl(path, arg0, ..., NULL)`, `execlp` (searches PATH), `execv(path, argv[])`, `execvp`.
- `wait(&s)` waits for any child. `waitpid(pid, &s, WNOHANG)` is non-blocking. `waitpid(-1, &s, 0)` is the same as `wait`.
- **After `fork()` memory is copied, not shared.** A global variable changed in the child is NOT seen by the parent. Use IPC.
- Zombie = child exited but the parent has not called `wait`. Orphan = parent died first (adopted by init/systemd).
- `fflush(stdout)` before `fork()` to avoid duplicated buffered output.

---

## 4. Scheduling

### In C
```c
#include <sched.h>

sched_yield();                                   /* give up the CPU voluntarily */

struct sched_param sp = { .sched_priority = 10 };      /* RT priority: 1..99 */
if (sched_setscheduler(0, SCHED_FIFO, &sp) == -1)      /* 0 = this process; needs root */
    perror("sched_setscheduler");

int pol = sched_getscheduler(0);                 /* SCHED_OTHER=0 FIFO=1 RR=2 BATCH=3 IDLE=5 */
```
- `SCHED_OTHER` (default, CFS/EEVDF): priority must be 0; influence it with **nice** (-20..19).
- `SCHED_FIFO` / `SCHED_RR`: real-time, priority 1..99, need root or `CAP_SYS_NICE`.
- `sched_yield` mostly matters between tasks of the same priority.

### Commands
```bash
ps -eo pid,cls,pri,ni,rtprio,comm      # CLS: TS=OTHER  FF=FIFO  RR=RR  B=BATCH  IDL=IDLE
chrt -p PID                            # SHOW current policy and priority of a process
chrt -m                                # show min/max priority for each policy
sudo chrt -f -p 50 PID                 # change running process to SCHED_FIFO, prio 50
sudo chrt -r -p 50 PID                 # SCHED_RR
sudo chrt -o -p 0  PID                 # back to SCHED_OTHER (priority must be 0)
sudo chrt -b -p 0  PID                 # SCHED_BATCH
sudo chrt -f 50 ./prog                 # start a NEW program under SCHED_FIFO 50

nice -n 10 ./prog                      # start with nice 10 (lower priority)
renice -n 5 -p PID                     # change nice of running process
renice -n -5 -p PID                    # negative = higher priority, needs sudo
renice -n 5 -u username                # all processes of a user
taskset -cp PID                        # show CPU affinity;  taskset -c 0 ./prog = pin to CPU 0
```
Normal users can only *increase* nice (lower priority), never decrease it.

---

## 5. Signals and Process Commands

```c
#include <signal.h>

kill(pid, SIGTERM);                       /* send a signal to a process */
kill(pid, 0);                             /* signal 0 = only check that pid exists */
raise(SIGUSR1);                           /* signal yourself */

volatile sig_atomic_t stop = 0;
void handler(int sig) { stop = 1; }       /* keep handlers tiny */

struct sigaction sa = {0};
sa.sa_handler = handler;
sigemptyset(&sa.sa_mask);
sigaction(SIGINT, &sa, NULL);             /* or: signal(SIGINT, handler); */
while (!stop) pause();                    /* sleep until a signal arrives */
```
Common signals (x86 Linux): `SIGINT 2` (Ctrl-C), `SIGKILL 9` (cannot be caught), `SIGUSR1 10`, `SIGSEGV 11`, `SIGTERM 15` (default for `kill`), `SIGCHLD 17`, `SIGCONT 18`, `SIGSTOP 19` (cannot be caught).

### ps
```bash
ps                              # processes of current terminal
ps -ef                          # every process, full format (PID, PPID, CMD)
ps aux                          # BSD style, shows %CPU %MEM STAT
ps -ef --forest                 # parent/child tree        (also: pstree -p)
ps -p PID -o pid,ppid,stat,ni,pri,cmd
ps -o pid,ppid,cmd --ppid PID   # children of PID
ps -eLf                         # show THREADS (LWP column = thread id, NLWP = thread count)
ps -T -p PID                    # threads of one process
ps -C name                      # by command name
ps -el | grep ' Z '             # zombies (STAT Z, shown as <defunct>)
```
STAT codes: `R` running · `S` sleeping · `D` uninterruptible I/O · `T` stopped · `Z` zombie · `+` foreground · `l` multi-threaded.

### top
```bash
top                 # live view
top -p PID          # watch one process
top -H -p PID       # per-THREAD view
top -b -n 1         # one snapshot, batch mode (good for scripts/screenshots)
```
Interactive keys: `k` kill · `r` renice · `H` toggle threads · `M` sort by memory · `P` sort by CPU · `1` per-CPU · `u` filter by user · `q` quit.

### kill / pkill
```bash
kill -l                       # list signals
kill PID                      # SIGTERM (15), polite
kill -9 PID                   # SIGKILL, cannot be ignored
kill -SIGSTOP PID             # pause   |   kill -SIGCONT PID   # resume
kill -0 PID                   # test if process exists (exit status 0 = yes)
pgrep -l name                 # find pids by name
pkill name                    # kill by name (SIGTERM)
pkill -9 -f "full command"    # -f matches the whole command line
pkill -SIGUSR1 name           # send any signal
pkill -P PARENT_PID           # kill children of a parent
pkill -u username             # all processes of a user
killall name                  # same idea as pkill
```

---

## 6. Pipes and Named Pipes (FIFO)

### Unnamed pipe (related processes only: parent/child)
```c
int fd[2];                       /* fd[0] = read end, fd[1] = write end */
pipe(fd);                        /* create BEFORE fork */
if (fork() == 0) {               /* child reads */
    close(fd[1]);                /* close unused write end */
    char buf[64]; ssize_t n = read(fd[0], buf, sizeof(buf)-1); buf[n] = 0;
    close(fd[0]); exit(0);
} else {                         /* parent writes */
    close(fd[0]);
    write(fd[1], "hello", 6);
    close(fd[1]);                /* reader sees EOF only when ALL write ends are closed */
    wait(NULL);
}
```

### Named pipe / FIFO (any unrelated processes, appears as a file)
```c
#include <sys/stat.h>
#include <fcntl.h>

#define FIFO "/tmp/myfifo"
if (mkfifo(FIFO, 0666) == -1 && errno != EEXIST) perror("mkfifo");

/* WRITER */
int fd = open(FIFO, O_WRONLY);          /* BLOCKS until a reader opens it */
write(fd, "hi", 3);
close(fd);

/* READER */
int fd = open(FIFO, O_RDONLY);          /* BLOCKS until a writer opens it */
char buf[64]; ssize_t n = read(fd, buf, sizeof(buf)-1);   /* n == 0 -> writer closed (EOF) */
close(fd);

unlink(FIFO);                           /* remove the FIFO file when done */
```
Shell: `mkfifo f`, `echo hi > f &`, `cat f`, `ls -l f` (type shows `p`), `rm f`.
Exam trap: both sides' `open()` block, so make sure the other side WILL open it, or you deadlock.

---

## 7. Message Queues

### POSIX
```c
#include <mqueue.h>

struct mq_attr attr = { .mq_flags = 0, .mq_maxmsg = 10, .mq_msgsize = 256, .mq_curmsgs = 0 };
mqd_t mq = mq_open("/myq", O_CREAT | O_RDWR, 0666, &attr);   /* name starts with '/' */
if (mq == (mqd_t)-1) perror("mq_open");

mq_send(mq, "hello", strlen("hello") + 1, 1);      /* (mq, data, len, priority) */

char buf[256];                                     /* buffer MUST be >= mq_msgsize */
unsigned prio;
ssize_t n = mq_receive(mq, buf, sizeof(buf), &prio);   /* higher priority comes out first */

mq_close(mq);
mq_unlink("/myq");                                 /* destroy */
```

### System V
```c
#include <sys/ipc.h>
#include <sys/msg.h>

struct my_msg { long mtype; char mtext[128]; };    /* first field MUST be long mtype > 0 */

key_t key = ftok("/tmp", 'Q');                     /* path must exist; same args = same key */
int qid = msgget(key, IPC_CREAT | 0666);

struct my_msg m = { .mtype = 1 };
strcpy(m.mtext, "hello");
msgsnd(qid, &m, strlen(m.mtext) + 1, 0);           /* size EXCLUDES mtype */

msgrcv(qid, &m, sizeof(m.mtext), 1, 0);            /* type 1 only; type 0 = first of any type */

msgctl(qid, IPC_RMID, NULL);                       /* destroy */
```

---

## 8. Shared Memory

### POSIX
```c
#include <sys/mman.h>

const char *NAME = "/myshm";
size_t SIZE = 4096;

int fd = shm_open(NAME, O_CREAT | O_RDWR, 0666);   /* creator */
ftruncate(fd, SIZE);                               /* set size (new object is 0 bytes) */
char *p = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
if (p == MAP_FAILED) perror("mmap");
close(fd);                                         /* mapping stays valid */

strcpy(p, "hello via shm");                        /* use like normal memory */

munmap(p, SIZE);
shm_unlink(NAME);                                  /* destroy (once, by one process) */
```
The other process: `shm_open(NAME, O_RDWR, 0666)` (no `O_CREAT`) then `mmap` the same way.

### System V
```c
#include <sys/ipc.h>
#include <sys/shm.h>

key_t key = ftok("/tmp", 'M');
int shmid = shmget(key, 4096, IPC_CREAT | 0666);   /* IPC_PRIVATE works for parent/child only */
char *p = shmat(shmid, NULL, 0);                   /* attach; (void*)-1 on error */
if (p == (char *)-1) perror("shmat");

strcpy(p, "hello via shm");

shmdt(p);                                          /* detach */
shmctl(shmid, IPC_RMID, NULL);                     /* mark for destruction */
```
Shared memory has **no built-in synchronization**, so pair it with a semaphore or process-shared mutex.

---

## 9. Semaphores

### POSIX unnamed (threads, or related processes via shared memory)
```c
#include <semaphore.h>

sem_t s;
sem_init(&s, 0, 1);      /* (sem, pshared, initial)  pshared=0: threads of one process
                            pshared=1: processes, but the sem_t MUST live in shared memory */
sem_wait(&s);            /* P: decrement, blocks if 0 */
/* critical section */
sem_post(&s);            /* V: increment, wakes a waiter */
sem_destroy(&s);
```
Also: `sem_trywait`, `sem_timedwait`, `sem_getvalue(&s, &val)`.

### POSIX named (unrelated processes, no shared memory needed)
```c
sem_t *s = sem_open("/mysem", O_CREAT, 0666, 1);   /* name, flags, mode, initial value */
if (s == SEM_FAILED) perror("sem_open");
sem_wait(s);
/* critical section */
sem_post(s);
sem_close(s);            /* each process */
sem_unlink("/mysem");    /* once, to remove (file appears as /dev/shm/sem.mysem) */
```

### System V
```c
#include <sys/sem.h>

union semun { int val; struct semid_ds *buf; unsigned short *array; };   /* YOU must define this */

key_t key = ftok("/tmp", 'S');
int semid = semget(key, 1, IPC_CREAT | 0666);      /* a set of 1 semaphore */
union semun u = { .val = 1 };
semctl(semid, 0, SETVAL, u);                       /* init semaphore #0 to 1 */

struct sembuf P = { 0, -1, 0 };                    /* { sem_num, sem_op, sem_flg } */
struct sembuf V = { 0, +1, 0 };
semop(semid, &P, 1);                               /* wait / lock */
/* critical section */
semop(semid, &V, 1);                               /* signal / unlock */

semctl(semid, 0, IPC_RMID);                        /* destroy */
```

---

## 10. Threads, Mutex, Condition Variables, RW Locks

```c
#include <pthread.h>

void *worker(void *arg) {
    int id = *(int *)arg;
    printf("thread %d\n", id);
    return NULL;                     /* or pthread_exit(NULL) */
}

pthread_t t;
int id = 1;
int rc = pthread_create(&t, NULL, worker, &id);   /* returns error number (NOT -1, no errno) */
if (rc != 0) fprintf(stderr, "pthread_create: %s\n", strerror(rc));
pthread_join(t, NULL);                            /* wait for thread to finish */
/* pthread_detach(t) = don't join, resources freed on exit; pthread_self() = my thread id */
```
Threads share globals, heap and open files. Each has its own stack. Don't pass the address of a loop variable to several threads; use an array of ids.

### Mutex
```c
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;    /* or pthread_mutex_init(&m, NULL) */
pthread_mutex_lock(&m);
/* critical section */
pthread_mutex_unlock(&m);
pthread_mutex_trylock(&m);                        /* returns EBUSY if already locked */
pthread_mutex_destroy(&m);
```
Mutex shared between **processes**: place it in shared memory and set the attribute:
```c
pthread_mutexattr_t a;
pthread_mutexattr_init(&a);
pthread_mutexattr_setpshared(&a, PTHREAD_PROCESS_SHARED);
pthread_mutex_init(&shm->lock, &a);
```

### Condition variable (producer-consumer with a counter)
```c
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_full  = PTHREAD_COND_INITIALIZER;
int count = 0;                                    /* N = buffer capacity */

/* producer */
pthread_mutex_lock(&m);
while (count == N)                                /* ALWAYS a while loop, never if */
    pthread_cond_wait(&not_full, &m);             /* atomically unlocks m and sleeps */
/* put item */ count++;
pthread_cond_signal(&not_empty);
pthread_mutex_unlock(&m);

/* consumer */
pthread_mutex_lock(&m);
while (count == 0)
    pthread_cond_wait(&not_empty, &m);
/* take item */ count--;
pthread_cond_signal(&not_full);
pthread_mutex_unlock(&m);
```
`pthread_cond_broadcast` wakes all waiters. Always hold the mutex when calling `cond_wait`.

### Read-write lock (many readers OR one writer)
```c
pthread_rwlock_t rw = PTHREAD_RWLOCK_INITIALIZER;

pthread_rwlock_rdlock(&rw);    /* several threads can hold this together */
/* read shared data */
pthread_rwlock_unlock(&rw);

pthread_rwlock_wrlock(&rw);    /* exclusive */
/* modify shared data */
pthread_rwlock_unlock(&rw);

pthread_rwlock_destroy(&rw);
```

### Bounded buffer with two semaphores + mutex (the standard recipe)
```
sem empty = N   (free slots)      sem full = 0   (filled slots)      mutex m

producer:  sem_wait(empty);  lock(m);  insert;  unlock(m);  sem_post(full);
consumer:  sem_wait(full);   lock(m);  remove;  unlock(m);  sem_post(empty);
```
The semaphores handle "buffer full/empty". The mutex protects the buffer indices.

---

## 11. Sockets

### TCP server → client flow
```
SERVER: socket → (setsockopt) → bind → listen → accept → read/write → close
CLIENT: socket → connect → write/read → close
```
```c
/* ---------- SERVER ---------- */
int srv = socket(AF_INET, SOCK_STREAM, 0);
int opt = 1;
setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));   /* avoid "address in use" */

struct sockaddr_in addr = {0};
addr.sin_family      = AF_INET;
addr.sin_addr.s_addr = htonl(INADDR_ANY);
addr.sin_port        = htons(5000);                              /* host -> network byte order */
bind(srv, (struct sockaddr *)&addr, sizeof(addr));
listen(srv, 5);

int cli = accept(srv, NULL, NULL);                               /* blocks for a client */
char buf[256];
ssize_t n = read(cli, buf, sizeof(buf) - 1);  buf[n] = '\0';     /* or recv() */
write(cli, "ack", 3);                                            /* or send() */
close(cli); close(srv);

/* ---------- CLIENT ---------- */
int s = socket(AF_INET, SOCK_STREAM, 0);
struct sockaddr_in sa = {0};
sa.sin_family = AF_INET;
sa.sin_port   = htons(5000);
inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr);
connect(s, (struct sockaddr *)&sa, sizeof(sa));
write(s, "hi", 2);
char reply[16]; read(s, reply, sizeof(reply));
close(s);
```
- Serve many clients: `fork()` per accepted connection (use `signal(SIGCHLD, SIG_IGN)` to avoid zombies) or one thread per client.
- **UDP**: `socket(AF_INET, SOCK_DGRAM, 0)`, then `sendto()` / `recvfrom()`. No listen/accept/connect needed.
- **UNIX-domain** (same machine): `AF_UNIX`, `struct sockaddr_un` (`sun_family`, `sun_path = "/tmp/sock"`), `unlink(path)` before `bind`.
- `socketpair(AF_UNIX, SOCK_STREAM, 0, sv)` before `fork()` gives a two-way parent/child channel.

---

## 12. Linux Commands: Memory, ASLR, /proc, IPC Objects

### ASLR (Address Space Layout Randomization)
```bash
cat /proc/sys/kernel/randomize_va_space          # 0 = off | 1 = stack/mmap/vdso | 2 = full (+heap), default
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space     # disable
sudo sysctl -w kernel.randomize_va_space=2                # enable full
setarch $(uname -m) -R ./prog                             # disable ASLR for ONE run only
```
`sudo echo 0 > file` does NOT work because the redirect runs as your user. Use `tee` or `sysctl -w`.
Demo: run a program that prints the address of a stack variable and of `malloc()` memory a few times. With 2 the addresses change each run, with 0 they stay the same.

### /proc/PID/maps (memory layout)
```bash
cat /proc/self/maps             # maps of the cat process itself
cat /proc/$$/maps               # your current shell
cat /proc/PID/maps              # a running process (get PID from ps / pgrep)
pmap PID                        # friendlier view;  pmap -x PID shows sizes
```
Each line: `start-end  perms  offset  dev  inode  pathname`
- perms: `r` read, `w` write, `x` execute, `p` private / `s` shared
- Look for `[heap]`, `[stack]`, `[vdso]`, the program binary (code `r-xp`, data `rw-p`), `libc.so`, and shared mappings (`/dev/shm/...` for POSIX shm, `SYSV....` for System V shm).

Other useful `/proc` files: `/proc/PID/status` (state, threads, VmRSS), `/proc/PID/fd/` (open files, incl. pipes and sockets), `/proc/PID/task/` (one dir per thread), `/proc/PID/cmdline`, `/proc/meminfo`.

### Memory overcommit
```bash
cat /proc/sys/vm/overcommit_memory        # 0 = heuristic (default) | 1 = always allow | 2 = never overcommit
cat /proc/sys/vm/overcommit_ratio         # used in mode 2: limit = swap + ratio% of RAM (default 50)
grep -i commit /proc/meminfo              # CommitLimit (max) and Committed_AS (promised so far)
sudo sysctl -w vm.overcommit_memory=1     # change (or echo 1 | sudo tee /proc/sys/vm/overcommit_memory)
free -h                                   # RAM and swap usage
```
Overcommit means `malloc()` may succeed even if there is not enough RAM, because pages are only allocated when touched. If memory then runs out, the kernel's **OOM killer** kills a process.
- Mode 0: obviously huge requests are refused. Mode 1: `malloc` basically never fails. Mode 2: `malloc` fails (returns NULL) once `Committed_AS` would exceed `CommitLimit`.
- OOM controls: `/proc/PID/oom_score` (higher = more likely to die), `/proc/PID/oom_score_adj` (-1000..1000).
```c
void *p = malloc(64UL * 1024 * 1024 * 1024);       /* 64 GB: compare result under mode 0, 1 and 2 */
printf("%s\n", p ? "malloc OK" : "malloc failed");
```

### IPC objects
```bash
# System V
ipcs            # all  (-m shared memory, -q message queues, -s semaphores)
ipcs -m
ipcrm -m SHMID  # remove shm;  -q MSQID  message queue;  -s SEMID  semaphore
ipcrm -M KEY    # by key instead of id

# POSIX
ls -l /dev/shm                # POSIX shm objects and named semaphores (sem.NAME)
ls /dev/mqueue                # POSIX queues (if not visible: sudo mount -t mqueue none /dev/mqueue)
cat /dev/mqueue/myq           # shows QSIZE and queue info

# FIFO and sockets
mkfifo f ; ls -l f            # type 'p'
ss -tulnp                     # listening TCP/UDP sockets with owning process  (older: netstat -tulnp)
ss -x                         # UNIX-domain sockets
lsof -i :5000                 # who uses port 5000
nc -l 5000   /   nc 127.0.0.1 5000     # quick TCP test server / client
```

### Threads
```bash
ps -eLf | grep prog       # LWP = thread id
ps -T -p PID
top -H -p PID
ls /proc/PID/task
```

---

## 13. Sample Question: Full Solution

**Q1.** Three children from one parent. Only one child spawns two threads. Named pipe between the parent and one child. POSIX semaphores + mutex for the threads. Producer-consumer between the two threads. Graceful termination.

### Design
```
parent ──fork──► child 0 ── creates ──► producer thread ──┐ bounded buffer
   │                 ▲                  consumer thread ◄─┘ (2 semaphores + mutex)
   │                 └── reads item count from FIFO
   ├──fork──► child 1  (plain)
   ├──fork──► child 2  (plain)
   └── writes item count to FIFO, waitpid() x3, unlink FIFO
```

### Code (`lab_q1.c`)
```c
/* Build: gcc -Wall -Wextra -pthread lab_q1.c -o lab_q1 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>

#define FIFO_PATH "/tmp/lab_q1_fifo"
#define BUF_SIZE  5
#define NUM_CHILD 3

/* ---- shared by the two threads (exists only inside child 0) ---- */
static int buffer[BUF_SIZE];
static int in_idx = 0, out_idx = 0;
static int total_items = 0;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static sem_t empty_slots;          /* free slots,   starts at BUF_SIZE */
static sem_t full_slots;           /* filled slots, starts at 0        */

static void *producer(void *arg)
{
    (void)arg;
    for (int i = 1; i <= total_items; i++) {
        sem_wait(&empty_slots);                    /* wait for a free slot */
        pthread_mutex_lock(&mtx);
        buffer[in_idx] = i * 10;
        printf("  [producer] produced %d (slot %d)\n", buffer[in_idx], in_idx);
        in_idx = (in_idx + 1) % BUF_SIZE;
        pthread_mutex_unlock(&mtx);
        sem_post(&full_slots);                     /* announce a filled slot */
    }
    return NULL;
}

static void *consumer(void *arg)
{
    (void)arg;
    for (int i = 1; i <= total_items; i++) {
        sem_wait(&full_slots);                     /* wait for an item */
        pthread_mutex_lock(&mtx);
        int item = buffer[out_idx];
        printf("  [consumer] consumed %d (slot %d)\n", item, out_idx);
        out_idx = (out_idx + 1) % BUF_SIZE;
        pthread_mutex_unlock(&mtx);
        sem_post(&empty_slots);                    /* announce a free slot */
    }
    return NULL;
}

static void child_with_threads(void)
{
    char msg[32] = {0};

    int fd = open(FIFO_PATH, O_RDONLY);            /* blocks until parent opens for writing */
    if (fd < 0) { perror("child open fifo"); exit(1); }
    if (read(fd, msg, sizeof(msg) - 1) <= 0) { perror("child read fifo"); exit(1); }
    close(fd);

    total_items = atoi(msg);
    printf("[child 0] received item count %d from parent via FIFO\n", total_items);
    fflush(stdout);

    sem_init(&empty_slots, 0, BUF_SIZE);           /* pshared = 0: threads only */
    sem_init(&full_slots, 0, 0);

    pthread_t prod, cons;
    int rc;
    if ((rc = pthread_create(&prod, NULL, producer, NULL)) != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(rc)); exit(1);
    }
    if ((rc = pthread_create(&cons, NULL, consumer, NULL)) != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(rc)); exit(1);
    }

    pthread_join(prod, NULL);                      /* graceful: wait for both threads */
    pthread_join(cons, NULL);

    sem_destroy(&empty_slots);                     /* clean up sync objects */
    sem_destroy(&full_slots);
    pthread_mutex_destroy(&mtx);

    printf("[child 0] both threads finished, exiting\n");
    exit(0);
}

static void plain_child(int id)
{
    sleep(1);
    printf("[child %d] done\n", id);
    exit(10 + id);                                 /* distinct exit code, parent will read it */
}

int main(void)
{
    pid_t pids[NUM_CHILD];

    unlink(FIFO_PATH);                             /* remove a stale FIFO from an earlier run */
    if (mkfifo(FIFO_PATH, 0666) == -1) { perror("mkfifo"); return 1; }

    printf("[parent] pid=%d\n", getpid());
    fflush(stdout);                                /* avoid duplicated buffered output after fork */

    for (int i = 0; i < NUM_CHILD; i++) {
        pids[i] = fork();
        if (pids[i] < 0) { perror("fork"); unlink(FIFO_PATH); return 1; }
        if (pids[i] == 0) {                        /* ---- child ---- */
            printf("[child %d] pid=%d ppid=%d\n", i, getpid(), getppid());
            fflush(stdout);
            if (i == 0) child_with_threads();      /* only child 0 creates threads */
            else        plain_child(i);
            /* not reached: both functions call exit() */
        }
    }

    /* ---- parent: send the item count to child 0 through the FIFO ---- */
    int fd = open(FIFO_PATH, O_WRONLY);            /* blocks until child 0 opens for reading */
    if (fd < 0) {
        perror("parent open fifo");
    } else {
        const char *msg = "8";
        if (write(fd, msg, strlen(msg) + 1) < 0) perror("parent write fifo");
        close(fd);
    }

    /* ---- reap every child: no zombies ---- */
    for (int i = 0; i < NUM_CHILD; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) == -1) { perror("waitpid"); continue; }
        if (WIFEXITED(status))
            printf("[parent] child %d (pid %d) exited with code %d\n",
                   i, pids[i], WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("[parent] child %d (pid %d) killed by signal %d\n",
                   i, pids[i], WTERMSIG(status));
    }

    unlink(FIFO_PATH);                             /* remove the FIFO file */
    printf("[parent] all children reaped, exiting\n");
    return 0;
}
```

### Run and verify
```bash
gcc -Wall -Wextra -pthread lab_q1.c -o lab_q1 && ./lab_q1

# in a 2nd terminal while it runs (put a sleep() in child 0 if it is too fast):
ps -ef --forest | grep lab_q1        # 1 parent + 3 children
ps -T -p <child0_pid>                # child 0 shows main thread + 2 threads
ls -l /tmp/lab_q1_fifo               # type 'p'
```

### Why it satisfies each requirement
| Requirement | Where |
|---|---|
| 3 children, common parent | `fork()` loop in `main`, children `exit()` so they never fork again |
| Threads from only one child | `child_with_threads()` is called only when `i == 0` |
| Named pipe parent↔child | `mkfifo` + `open/write` in parent, `open/read` in child 0 |
| POSIX semaphores + mutex | `empty_slots`, `full_slots` (`sem_init`), `mtx` (`pthread_mutex_t`) |
| Producer-consumer | `producer()` / `consumer()` on a circular bounded buffer |
| Graceful termination | `pthread_join` ×2, `sem_destroy`, `waitpid` ×3, `unlink(FIFO)` |

### Variations the examiner might ask for
- **Use a condition variable instead of semaphores**: use the Section 10 pattern.
- **Use a POSIX message queue instead of the FIFO**: Section 7.
- **FIFO between parent and a *different* child**: the same code, but that child does `open/read`.
- **Child to parent direction**: the child opens `O_WRONLY`, the parent `O_RDONLY`.
- **Use System V semaphores between the processes**: Section 9, System V.
- **Sum/print result from threads**: protect the shared variable with the mutex, then print after `pthread_join`.

---

## 14. Exam-Day Checklist and Common Mistakes

**Before coding**
- Pick the headers from Section 1 first, then compile early and often with `-Wall -pthread`.

**Processes**
- Child must `exit()` / `_exit()`, otherwise it runs the parent's code and the loop forks again.
- `execl` argument list must end with `(char *)NULL`. Code after `exec` runs only on failure.
- Memory is **not** shared after `fork`, so use IPC for data.
- `wait` for *every* child. Print `WEXITSTATUS` only if `WIFEXITED` is true.

**IPC**
- FIFO `open()` blocks until the other end opens. Mind the order and avoid deadlock.
- POSIX names must start with `/`. System V needs `ftok()` on an **existing** path.
- SysV message size in `msgsnd/msgrcv` **excludes** `mtype`. `mq_receive` buffer must be ≥ `mq_msgsize`.
- `union semun` must be defined by you on Linux.
- Unnamed POSIX semaphore across processes needs `pshared=1` and a `sem_t` placed in shared memory.
- Clean up every time: `unlink` / `shm_unlink` / `mq_unlink` / `sem_unlink` / `IPC_RMID`. Check leftovers with `ipcs`, `ls /dev/shm`, `ls /dev/mqueue`.

**Threads**
- Condition variable waits go in a `while` loop and are called with the mutex held.
- Lock and unlock in the same thread. Never `return` while holding a lock.
- Pass each thread its own argument (array element, not the loop variable's address).
- `pthread_create` returns an error number, not -1.
- Always `pthread_join` (or detach) so the process doesn't exit while threads run.
- `sleep()` is not synchronization. Use semaphores, mutexes or condition variables.

**Privileges**
- `chrt` real-time policies, negative `renice`, writing `/proc/sys/...` and `sysctl -w` all need `sudo`.

**Graceful shutdown pattern**
1. Signal handler sets a `volatile sig_atomic_t stop` flag (SIGINT/SIGTERM).
2. Workers check the flag, finish their loop, then exit.
3. Join threads → destroy mutexes/semaphores → close fds → wait for children → unlink named IPC objects.
